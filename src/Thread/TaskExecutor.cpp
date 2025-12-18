#include "TaskExecutor.h"
#include "Poller/EventPoller.h"
#include "Util/onceToken.h"
#include "Util/TimeTicker.h"
#include "semaphore.h"

using namespace std;

namespace FFZKit {

ThreadLoadCounter::ThreadLoadCounter(uint64_t max_size, uint64_t max_usec) {
	_last_sleep_time = _last_wake_time = getCurrentMicrosecond();
	_max_size = max_size;
    _max_usec = max_usec;
}

void ThreadLoadCounter::startSleep() {
	lock_guard<mutex> lck(_mtx);
	_sleeping = true;
    auto current_time = getCurrentMicrosecond();
	auto run_time = current_time - _last_wake_time;
	_last_sleep_time = current_time;
	_time_list.emplace_back(run_time, false);
	if (_time_list.size() > _max_size) {
        _time_list.pop_front();
	}
}

//休眠唤醒,结束休眠
void ThreadLoadCounter::sleepWakeUp() {
	lock_guard<mutex> lck(_mtx);
	_sleeping = false;
	auto current_time = getCurrentMicrosecond();
	auto sleep_time = current_time - _last_sleep_time;
	_last_wake_time = current_time;
    _time_list.emplace_back(sleep_time, true);
    if (_time_list.size() > _max_size) {
        _time_list.pop_front();
	}
}

int ThreadLoadCounter::load() {
	uint64_t totalSleepTime = 0;
	uint64_t totalRunTime = 0;

	lock_guard<mutex> lck(_mtx);
	_time_list.for_each([&](const TimeRecord& rcd) {
		if (rcd._sleep) {
			totalSleepTime += rcd._time;
		}
		else {
			totalRunTime += rcd._time;
		}
		});

	if (_sleeping) {
		totalSleepTime += (getCurrentMicrosecond() - _last_sleep_time);
	}
	else {
		totalRunTime += (getCurrentMicrosecond() - _last_wake_time);
	}

	uint64_t totalTime = totalRunTime + totalSleepTime;

	while ((_time_list.size() > 0) && (totalTime > _max_usec || _time_list.size() > _max_size)) {
		TimeRecord& rcd = _time_list.front();
		if (rcd._sleep) {
			totalSleepTime -= rcd._time;
		} else {
			totalRunTime -= rcd._time;
		}
		totalTime -= rcd._time;
		_time_list.pop_front();
	}

	if (0 == totalTime) {
		return 0;
	}
	return (int)(totalRunTime * 100 / totalTime);
}

///////////////////////////////////////////////////////////////////////////////////

Task::Ptr TaskExecutorInterface::async_first(TaskIn task, bool may_sync) {
	return async(task, may_sync);
}

void TaskExecutorInterface::sync(const TaskIn& task) {
	semaphore sem;
	auto ret = async([&]() {
		OnceToken token(nullptr, [&]() {
			//通过RAII原理防止抛异常导致不执行这句代码
			sem.post();
		});
        task();
	});

	if (ret && *ret) {
        sem.wait();
	}
}

void TaskExecutorInterface::sync_first(const TaskIn& task) {
	semaphore sem;
	auto ret = async_first([&]() {
		OnceToken token(nullptr, [&]() {
			//通过RAII原理防止抛异常导致不执行这句代码
			sem.post();
			});
		task();
	});

	if (ret && *ret) {
		sem.wait();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

TaskExecutor::TaskExecutor(uint64_t max_size, uint64_t max_usec): ThreadLoadCounter(max_size, max_usec) {}

/////////////////////////////////////////////////////////////////////////////////////////////

TaskExecutor::Ptr TaskExecutorGetterImp::getExecutor() {
	if(threads_.empty()) {
		return nullptr;
	}

	auto thread_pos = thread_pos_;
	if(thread_pos >= threads_.size()) {
		thread_pos = 0;
	}

	TaskExecutor::Ptr executor_min_load = threads_[thread_pos];
	auto min_load = executor_min_load->load();

	for(int i=0; i< threads_.size(); ++i) {
		++thread_pos;
		if(thread_pos >= threads_.size()) {
			thread_pos = 0;
		}

		auto executor = threads_[thread_pos];
		if(!executor) {
			continue;
		}
		
		auto load = executor->load();
		if(load < min_load) {
			min_load = load;
			executor_min_load = executor;
		}

		if(0 == min_load) {
			break; 
		}
	}
	thread_pos_ = thread_pos;
	return executor_min_load;
}

std::vector<int> TaskExecutorGetterImp::getExecutorLoad() {
	vector<int> vec(threads_.size());
	int i = 0;
	for(auto& executor : threads_) {
		vec[i++] = executor ? executor->load() : 0;
	}
	return vec;
}

void TaskExecutorGetterImp::getExecutorDelay(const function<void(const vector<int>&)> &callback) {
	std::shared_ptr<vector<int> > delay_vec = std::make_shared<vector<int> >(threads_.size());
	
	std::shared_ptr<void> finished(nullptr, [callback, delay_vec](void*) {
		callback(*delay_vec);
	});

	int index = 0;
	for(auto &th : threads_) {
		std::shared_ptr<Ticker> delay_ticker = std::make_shared<Ticker>();
		th->async([finished, delay_vec, index, delay_ticker]() {
			(*delay_vec)[index] = (int)delay_ticker->elapsedTime();
		}, false);
		++index;
	}
}

void TaskExecutorGetterImp::for_each(const function<void(const TaskExecutor::Ptr&)> &callback) {
	for(auto& executor : threads_) {
		if(executor) {
			callback(executor);
		}
	}
}

size_t TaskExecutorGetterImp::getExecutorSize() const {
	return threads_.size();
}

size_t TaskExecutorGetterImp::addPoller(const string& name, size_t size, int priority, bool register_thread, bool enable_cpu_affinity) {
 	auto cpus = thread::hardware_concurrency();
    size = size > 0 ? size : cpus;

	for(int i = 0; i < size; ++i) {
		auto full_name = name +  "-" + to_string(i);
		auto cpu_idx = i % cpus;
		EventPoller::Ptr poller(new EventPoller(full_name));
		poller->runLoop(false, register_thread);

		poller->async([cpu_idx, enable_cpu_affinity, full_name, priority]() {
			//Set thread priority
			ThreadPool::setPriority((ThreadPool::Priority)priority);
			setThreadName(full_name.data());
			if (enable_cpu_affinity) {
				setThreadAffinity(cpu_idx);
			}
		});

		threads_.emplace_back(std::move(poller));
	}

	return size;
}

} // namespace FFZKit