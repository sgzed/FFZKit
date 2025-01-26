#ifndef FFZKIT_TASKEXECUTOR_H_
#define FFZKIT_TASKEXECUTOR_H_

#include <mutex>
#include <memory>
#include <functional>
#include "Util/List.h"
#include "Util/util.h"

namespace FFZKit {

//cpu负载计算器
class ThreadLoadCounter {
public:
	/* 
    * 构造函数
	* @param max_size 统计样本数量
	* @param max_usec 统计时间窗口,最近{max_usec}的cpu负载
	*/
	ThreadLoadCounter(uint64_t max_size, uint64_t max_usec);
	virtual ~ThreadLoadCounter() = default;

	//线程休眠
	void startSleep(); 

	//休眠唤醒,结束休眠
	void sleepWakeUp();

	/**
	 * 返回当前线程cpu使用率，范围为 0 ~ 100
	 * @return 当前线程cpu使用率
	 */
	int load();

private:
	struct TimeRecord {
		TimeRecord(uint64_t tm, bool slp) {
			_time = tm;
            _sleep = slp;
		}

		bool _sleep;
        uint64_t _time;
	};

private:
	bool _sleeping = true;
	uint64_t _last_sleep_time;
	uint64_t _last_wake_time;
	uint64_t _max_size;
	uint64_t _max_usec;
	std::mutex _mtx;
	List<TimeRecord> _time_list;
};

class TaskCancelable : noncopyable {
public:
	TaskCancelable() = default;
    virtual ~TaskCancelable() = default;
	virtual void cancel() = 0;
};

template<typename R, typename...ArgTypes>
class TaskCancelableImp; 

template<typename R, typename...ArgTypes>
class TaskCancelableImp<R(ArgTypes...)> : public TaskCancelable {
public:
	using Ptr = std::shared_ptr<TaskCancelableImp>;
	using func_type = std::function<R(ArgTypes...)>;

	virtual ~TaskCancelableImp() = default;

	template<typename FUNC>
	TaskCancelableImp(FUNC&& task) {
        _weakTask = _strongTask = std::make_shared<func_type>(std::forward<FUNC>(task));
	}

	void cancel() override {
		_strongTask = nullptr;
	}

    operator bool() const {
		return _strongTask && *_strongTask;
	}

	void operator=(std::nullptr_t) {
		_strongTask = nullptr;
	}

	R operator()(ArgTypes... args) const {
		auto strongTask = _weakTask.lock();
		if (strongTask && *strongTask) {
			return (*strongTask)(std::forward<ArgTypes>(args)...);
		}
		return defaultValue<R>();
	}

	template<typename T> 
	static typename std::enable_if<std::is_void<T>::value, void>::type 
	defaultValue() { }

	template<typename T>
	static typename std::enable_if<std::is_pointer<T>::value, T>::type
	defaultValue() { 
		return nullptr;
	}

	template<typename T>
	static typename std::enable_if<std::is_integral<T>::value, T>::type
		defaultValue() {
		return 0;
	}

protected:
	std::weak_ptr<func_type> _weakTask;
	std::shared_ptr<func_type> _strongTask;
};


using TaskIn = std::function<void()>;
using Task = TaskCancelableImp<void()>;


class TaskExecutorInterface {
public:
	TaskExecutorInterface() = default;
	virtual ~TaskExecutorInterface() = default;

	/**
	* 异步执行任务
	* @param task 任务
	* @param may_sync 是否允许同步执行该任务
	* @return 任务是否添加成功
	*/
    virtual Task::Ptr async(TaskIn task, bool may_sync = true) = 0;

	/**
	 * 最高优先级方式异步执行任务
	 * @param task 任务
	 * @param may_sync 是否允许同步执行该任务
	 * @return 任务是否添加成功
	 */
	virtual Task::Ptr async_first(TaskIn task, bool may_sync = true);

	// 同步执行任务
    void sync(const TaskIn& task);

	void sync_first(const TaskIn& task);
};

class TaskExecutor : public ThreadLoadCounter, public TaskExecutorInterface {
public:
	using Ptr = std::shared_ptr<TaskExecutor>;

	/**
	* 构造函数
	* @param max_size cpu负载统计样本数
	* @param max_usec cpu负载统计时间窗口大小
	*/
	TaskExecutor(uint64_t max_size = 32, uint64_t max_usec = 2 * 1000 * 1000);
	virtual ~TaskExecutor() = default;
};

class TaskExecutorGetter {
public:
	using Ptr = std::shared_ptr<TaskExecutorGetter>;

	virtual ~TaskExecutorGetter() = default;
	
	/**
	* 获取任务执行器
	* @return 任务执行器
	*/
	virtual TaskExecutor::Ptr getExecutor() = 0;

	// 获取执行器个数
    virtual size_t getExecutorSize() = 0;
};

class TaskExecutorGetterImp : public TaskExecutorGetter {
public:
    TaskExecutorGetterImp() = default;
    virtual ~TaskExecutorGetterImp() = default;

    // 根据线程负载情况，获取最空闲的任务执行器
	TaskExecutor::Ptr getExecutor() override;

    /**
    * 获取所有线程的负载率
    * @return 所有线程的负载率
    */
    std::vector<int> getExecutorLoad();


    /**
     * 获取所有线程任务执行延时，单位毫秒
     * 通过此函数也可以大概知道线程负载情况
     */
    void getExecutorDelay(const std::function<void(const std::vector<int> &)> &callback);

    size_t getExecutorSize() const override;

protected:
    size_t addPoller(const std::string &name, size_t size, int priority, bool register_thread, bool enable_cpu_affinity = true);

protected:
    size_t thread_pos_ = 0;
    std::vector<TaskExecutor::Ptr> threads_;
};

} //FFZKit

#endif // FFZKIT_TASKEXECUTOR_H_
