#include "TaskExecutor.h"
#include "Util/onceToken.h"
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


} // namespace FFZKit