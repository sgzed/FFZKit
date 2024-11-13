#ifndef FFZKIT_LIST_H
#define FFZKIT_LIST_H

#include <list>

namespace FFZKit {

template<typename T>
class List : public std::list<T> {
public:
    template<typename ...Args> 
    List(Args&&... args) : std::list<T>(std::forward<Args>(args)...) { }
    ~List() = default;

    void append(List<T> &other) {
        if(other.empty()) {
            return;
        }
        this->insert(this->end(), other.begin(), other.end());
        other.clear();
    }

    template<typename FUNC>
    void for_each(FUNC &&func) {
        for(auto &it : *this) {
            func(it);
        }
    }

    template<typename FUNC>
    void for_each(FUNC &&func) const {
        for(auto &it : *this) {
            func(it);
        }
    }
};

} // namespace FFZKit

#endif // FFZKIT_LIST_H