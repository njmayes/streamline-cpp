#pragma once

#include "slc/Coroutine/Enumerable.h"

#include <deque>

namespace slc {

    template<typename T,
        typename Allocator = std::allocator<T>>
    class Deque : public std::deque<T, Allocator>, public IEnumerable<T>
    {
    public:
        using std::deque<T, Allocator>::deque;
        using std::deque<T, Allocator>::begin;
        using std::deque<T, Allocator>::end;

        MAKE_RANGE_ENUMERABLE(Deque)
    };

}