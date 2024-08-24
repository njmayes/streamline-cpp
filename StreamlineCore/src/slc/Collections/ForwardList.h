#pragma once

#include "slc/Coroutine/Enumerable.h"

#include <forward_list>

namespace slc {

    template<typename T,
        typename Allocator = std::allocator<T>>
    class ForwardList : public std::forward_list<T, Allocator>, public IEnumerable<T>
    {
    public:
        using std::forward_list<T, Allocator>::forward_list;
        using std::forward_list<T, Allocator>::begin;
        using std::forward_list<T, Allocator>::end;

        MAKE_RANGE_ENUMERABLE(ForwardList)
    };

}