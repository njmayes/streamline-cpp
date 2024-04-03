#pragma once

#include "Coroutine/Enumerable.h"

#include <list>

namespace slc {

    template<typename T,
        typename Allocator = std::allocator<T>>
        class List : public std::list<T, Allocator>, public IEnumerable<T>
    {
    public:
        using std::list<T, Allocator>::list;
    };

}