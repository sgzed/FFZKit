//
// Created by FFZero on 2025-01-12.
//

#ifndef FFZKIT_THREADGROUP_H_
#define FFZKIT_THREADGROUP_H_

#include <stdexcept>
#include <thread>
#include <unordered_map>
#include <memory>

namespace FFZKit {

class ThreadGroup {
private:
    ThreadGroup(const ThreadGroup&) = delete;
    ThreadGroup& operator=(const ThreadGroup&) = delete;

public:
    ThreadGroup() = default;
    ~ThreadGroup() {
        threads_.clear();
    }

    bool is_this_thread_in() {
        auto thread_id = std::this_thread::get_id();
        if(thread_id_ == thread_id) {
            return true;
        }
        return threads_.find(thread_id) != threads_.end();
    }

    bool is_thread_in(std::thread* thrd) {
        if(!thrd) {
            return false;
        }
        auto it = threads_.find(thrd->get_id());
        return it != threads_.end();
    }

    template<typename F>
    std::thread *create_thread(F&& threadfunc) {
        auto thrd = std::make_shared<std::thread>(std::forward<F>(threadfunc));
        thread_id_ = thrd->get_id();
        threads_[thread_id_] = thrd;
        return thrd.get();
    }

    void remove_thread(std::thread* thrd) {
        if(!thrd) return;
        auto it = threads_.find(thrd->get_id());
        if(it != threads_.end()) {
            threads_.erase(it);
        }
    }

    void join_all() {
        if (is_this_thread_in()) {
            throw std::runtime_error("Trying joining itself in thread_group");
        }

       for(auto& it : threads_) {
           if(it.second->joinable()) {
               it.second->join();
           }
       }
       threads_.clear();
    }

    size_t size() const {
        return threads_.size();
    }

private:
    std::thread::id thread_id_;
    std::unordered_map<std::thread::id, std::shared_ptr<std::thread> > threads_;
};

} // FFZKit

#endif //FFZKIT_THREADGROUP_H_
