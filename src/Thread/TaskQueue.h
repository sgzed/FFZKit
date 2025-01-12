//
// Created by FFZero on 2025-01-12.
//

#ifndef FFZKIT_TASKQUEUE_H_
#define FFZKIT_TASKQUEUE_H_

#include <mutex>
#include "Util/List.h"
#include "semaphore.h"

namespace FFZKit {

template <typename T>
class TaskQueue {
public:
    //打入任务至列队
    template <typename C>
    void push_task(C&& task_func) {
        {
            std::lock_guard<decltype(mutex_)> lock(mutex_);
            queue_.emplace_back(std::forward<C>(task_func));
        }
        sem_.post();
    }

    template<typename C>
    void push_task_first(C&& task_func) {
        {
            std::lock_guard<decltype(mutex_)> lock(mutex_);
            queue_.emplace_front(task_func);
        }
        sem_.post();
    }

    void push_exit(size_t n) {
        sem_.post(n);
    }

    bool get_task(T &task) {
        sem_.wait();
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        if (queue_.empty()) {
            return false;
        }
        task = std::move(queue_.front());
        queue_.pop_front();
        return true;
    }

    size_t size() const {
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        return queue_.size();
    }

private:
    List<T> queue_;
    mutable std::mutex mutex_;
    semaphore sem_;
};

} // FFZKit


#endif //FFZKIT_TASKQUEUE_H_
