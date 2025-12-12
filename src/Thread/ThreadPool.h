//
// Created by FFZero on 2025-01-12.
//

#ifndef FFZKIT_THREADPOOL_H_
#define FFZKIT_THREADPOOL_H_

#include "ThreadGroup.h"
#include "TaskExecutor.h"
#include "TaskQueue.h"
#include "Util/util.h"
#include "Util/logger.h"

namespace FFZKit {

class ThreadPool : public TaskExecutor {
public:
    enum Priority {
        PRIORITY_LOWEST = 0,
        PRIORITY_LOW,
        PRIORITY_NORMAL,
        PRIORITY_HIGH,
        PRIORITY_HIGHEST
    };

    ThreadPool(int num = 1, Priority priority = PRIORITY_HIGHEST, bool auto_run = true,
               bool set_affinity = true, const std::string &pool_name = "thread_pool") {
        thread_num_ = num;
        on_setup_ = [pool_name, priority, set_affinity](int index) {
            std::string name = pool_name + '_' + std::to_string(index);
            setPriority(priority);
            setThreadName(name.data());
            if (set_affinity) {
                setThreadAffinity(index % std::thread::hardware_concurrency());
            }
        };

        logger_ = Logger::Instance().shared_from_this();
        if(auto_run) {
            start();
        }
    }

    ~ThreadPool() {
        shutdown();
        wait();
    }

    //把任务打入线程池并异步执行
    Task::Ptr async(TaskIn task, bool may_sync = true) override {
        if (may_sync && thread_group_.is_this_thread_in()) {
            task();
            return nullptr;
        }
        auto ret = std::make_shared<Task>(std::move(task));
        task_queue_.push_task(ret);
        return ret;
    }

    Task::Ptr async_first(TaskIn task, bool may_sync = true) override {
        if (may_sync && thread_group_.is_this_thread_in()) {
            task();
            return nullptr;
        }

        auto ret = std::make_shared<Task>(std::move(task));
        task_queue_.push_task_first(ret);
        return ret;
    }


    void start() {
        if (thread_num_ <= 0) {
            return;
        }
        size_t total = thread_num_ - thread_group_.size();
        for(size_t i = 0; i < total; ++i) {
            thread_group_.create_thread([this, i]() {
                run(i);
            });
        }
    }

    size_t size() {
        return task_queue_.size();
    }

    static bool setPriority(Priority priority = PRIORITY_NORMAL, std::thread::native_handle_type threadId = 0) {
        // set priority
    #if defined(_WIN32)
            static int Priorities[] = { THREAD_PRIORITY_LOWEST, THREAD_PRIORITY_BELOW_NORMAL, THREAD_PRIORITY_NORMAL, THREAD_PRIORITY_ABOVE_NORMAL, THREAD_PRIORITY_HIGHEST };
            if (priority != PRIORITY_NORMAL && SetThreadPriority(GetCurrentThread(), Priorities[priority]) == 0) {
                return false;
            }
            return true;
    #else
        static int Min = sched_get_priority_min(SCHED_FIFO);
        if (Min == -1) {
            return false;
        }
        static int Max = sched_get_priority_max(SCHED_FIFO);
        if (Max == -1) {
            return false;
        }
        static int Priorities[] = {Min, Min + (Max - Min) / 4, Min + (Max - Min) / 2, Min + (Max - Min) * 3 / 4, Max};

        if (threadId == 0) {
            threadId = pthread_self();
        }
        struct sched_param params;
        params.sched_priority = Priorities[priority];
        return pthread_setschedparam(threadId, SCHED_FIFO, &params) == 0;
    #endif
    }

private:
    void run(size_t index) {
        on_setup_(index);
        Task::Ptr task;
        while (true) {
            startSleep();
            if (!task_queue_.get_task(task)) {
                //空任务，退出线程
                break;
            }
            sleepWakeUp();
            try {
                (*task)();
                task = nullptr;
            } catch (std::exception &ex) {
                ErrorL << "ThreadPool catch a exception: " << ex.what();
            }
        }
    }

    void wait() {
        thread_group_.join_all();
    }

    void  shutdown() {
        task_queue_.push_exit(thread_num_);
    }

private:
    size_t thread_num_;
    Logger::Ptr logger_;
    ThreadGroup thread_group_;
    TaskQueue<Task::Ptr> task_queue_;
    std::function<void(int)> on_setup_;
};

} // namespace FFZKit

#endif //FFZKIT_THREADPOOL_H_
