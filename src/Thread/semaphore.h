//
// Created by Administrator on 2024-11-11.
//

#ifndef FFZKIT_SEMAPHORE_H
#define FFZKIT_SEMAPHORE_H

#include <mutex>
#include <condition_variable>

namespace FFZKit {

class semaphore {
public:
    semaphore() : _count(0) {}
    ~semaphore() = default;

    void post(size_t n = 1) {
        std::unique_lock<std::recursive_mutex> lock(_mutex);
        _count += n;
        if (n == 1) {
            _condition.notify_one();
        } else {
            _condition.notify_all();
        }
    }

    void wait() {
        std::unique_lock<std::recursive_mutex> lock(_mutex);
        while (_count == 0) {
            _condition.wait(lock);
        }
        --_count;
    }

private:
    size_t _count;
    std::recursive_mutex _mutex;
    std::condition_variable_any _condition;
};

} // namespace FFZKit


#endif //FFZKIT_SEMAPHORE_H
