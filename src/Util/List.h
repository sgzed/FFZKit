#ifndef FFZKIT_LIST_H
#define FFZKIT_LIST_H

#include <list.h>


namespace FFZKit {

template<typename T>
class List : public std::list<T> {
public:
    template<typename ...Args> 
    List(Args&&... args) : std::list<T>(std::forward<Args>(args)...) {
    }
    ~List() = default;

};


} // namespace FFZKit

#endif