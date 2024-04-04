#pragma once

#include "Coroutine/Enumerable.h"

#include <deque>

namespace slc {

    template<typename T,
        typename Allocator = std::allocator<T>>
        class Deque : public std::deque<T, Allocator>, public IEnumerable<T>
    {
    public:
        using std::deque<T, Allocator>::deque;

        MAKE_RANGE_ENUMERABLE(Deque)
    };

}