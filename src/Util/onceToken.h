//
// Created by Administrator on 2024-11-10.
//

#ifndef FFZKIT_ONCETOKEN_H
#define FFZKIT_ONCETOKEN_H

#include <functional>

namespace FFZKit {

class OnceToken {
public:
    using task = std::function<void(void)>;

    template<typename FUNC>
    OnceToken(const FUNC &onConstructed, const task& onDestructed = nullptr)  {
        onConstructed();
        _onDestructed = std::move(onDestructed);
    }

    OnceToken(std::nullptr_t, task onDestructed = nullptr) {
        _onDestructed = std::move(onDestructed);
    }

    ~OnceToken() {
        if(_onDestructed) {
            _onDestructed();
        }
    }

private:
    OnceToken() = delete;
    OnceToken(const OnceToken &) = delete;
    OnceToken(OnceToken &&) = delete;
    OnceToken &operator=(const OnceToken &) = delete;
    OnceToken &operator=(OnceToken &&) = delete;

private:
    task _onDestructed;
};

} // namespace FFZKit

#endif //FFZKIT_ONCETOKEN_H
