#ifndef FFZKIT_RESOURCEPOOL_H
#define FFZKIT_RESOURCEPOOL_H

#include <atomic>
#include <functional>
#include <memory>
#include <vector>

namespace FFZKit {

#if (defined(__GNUC__) && (__GNUC__ >= 5 || (__GNUC__ >= 4 && __GNUC_MINOR__ >= 9))) || defined(__clang__)             \
    || !defined(__GNUC__)
#define SUPPORT_DYNAMIC_TEMPLATE
#endif

template<typename C>
class ResourcePool_I;

template<typename C>
class ResourcePool;

template<typename C>
class shared_ptr_imp : public std::shared_ptr<C> { 
public:
    shared_ptr_imp() {}

    /**
     * Constructs a smart pointer
     * @param ptr Raw pointer
     * @param weakPool Circular pool managing this pointer
     * @param quit Whether to give up circular reuse
     */
    shared_ptr_imp(C *ptr, const std::weak_ptr<ResourcePool_I<C>> &weakPool, std::shared_ptr<std::atomic_bool> quit,
        const std::function<void(C *)> &on_recycle);

    void quit(bool flag = true) {
        if (quit_) {
           // quit_->store(flag);
            *quit_ = flag;
        }
    }

private:
    std::shared_ptr<std::atomic_bool> quit_;
};

template <typename C>
class ResourcePool_I : public std::enable_shared_from_this<ResourcePool_I<C>> {
public:
    using ValuePtr = shared_ptr_imp<C>;
    friend class ResourcePool<C>;
    friend class shared_ptr_imp<C>;

    ResourcePool_I() {
        alloc_ = []() -> C* { return new C(); };
    }

#if defined(SUPPORT_DYNAMIC_TEMPLATE)
    template<typename... ArgsType>
    ResourcePool_I(ArgsType&&... args) {
        alloc_ = [args...]() -> C* { return new C(args...); };
    }
#endif // defined(SUPPORT_DYNAMIC_TEMPLATE)

    ~ResourcePool_I() {
        for (auto ptr : free_list_) {
            delete ptr;
        }
    }

    void setSize(size_t size) {
        pool_size_ = size;
        free_list_.reserve(size);
    }

    ValuePtr obtain(const std::function<void(C *)> &on_recycle) {
        return ValuePtr(getPtr(), weak_self_, std::make_shared<std::atomic_bool>(false), on_recycle);
    }

    std::shared_ptr<C> obtain2() {
        auto weak_self = weak_self_;
        return std::shared_ptr<C>(getPtr(), [weak_self](C *obj) {
            auto strongPool = weak_self.lock();
            if( strongPool ) {
                strongPool->recycle(obj);
            } else {
                delete obj;
            }
        });
    }

private:
    void setup() {
        weak_self_ = this->shared_from_this();
    }

    C* getPtr() {
        C* ptr = nullptr;
        auto is_busy = busy_.test_and_set();
        if (!is_busy) {  // get lock
            if (!free_list_.empty()) {
                ptr = free_list_.back();
                free_list_.pop_back();
            } else {
                ptr = alloc_();
            }
            busy_.clear();
        } else {  // failed to get lock
            ptr = alloc_();
        }
        return ptr;
    }

    void recycle(C* ptr) {
        auto is_busy = busy_.test_and_set();
        if (!is_busy) {  // get lock
            if (free_list_.size() < pool_size_) {
                free_list_.emplace_back(ptr);
            } else {
                delete ptr;
            }
            busy_.clear();
        } else {  // failed to get lock
            delete ptr;
        }
    }

private:
    size_t pool_size_{8};
    std::vector<C*> free_list_;
    std::function<C*(void)> alloc_;
    std::atomic_flag busy_{ false };
    std::weak_ptr<ResourcePool_I<C>> weak_self_;
};

template <typename C>
class ResourcePool {
public:
    using ValuePtr = shared_ptr_imp<C>;

    ResourcePool() {
        pool_ = std::make_shared<ResourcePool_I<C>>();
        pool_->setup();
    }

#if defined(SUPPORT_DYNAMIC_TEMPLATE)
    template<typename... ArgsType>
    ResourcePool(ArgsType&&... args) {
        pool_ = std::make_shared<ResourcePool_I<C>>(std::forward<ArgsType>(args)...);
        pool_->setup();
    }
#endif // defined(SUPPORT_DYNAMIC_TEMPLATE)

    void setSize(size_t size) {
        pool_->setSize(size);
    }

    ValuePtr obtain(const std::function<void(C *)> &on_recycle = nullptr) {
        return pool_->obtain(on_recycle);
    }

    std::shared_ptr<C> obtain2() {
        return pool_->obtain2();
    }

private:
    std::shared_ptr<ResourcePool_I<C>> pool_;
};

template<typename C>
shared_ptr_imp<C>::shared_ptr_imp(C *ptr, const std::weak_ptr<ResourcePool_I<C>> &weakPool,
                                    std::shared_ptr<std::atomic_bool> quit, 
                                    const std::function<void(C *)> &on_recycle)
    : std::shared_ptr<C>(ptr, [weakPool, quit, on_recycle](C *obj) {
            if (on_recycle) {
                on_recycle(obj);
            }
            auto strongPool = weakPool.lock();
          if (strongPool && !(*quit) ) {
              strongPool->recycle(obj);
          } else {
            delete obj;
          }
      }) , quit_(std::move(quit)) {}

} // namespace FFZKit

#endif // FFZKIT_RESOURCEPOOL_H