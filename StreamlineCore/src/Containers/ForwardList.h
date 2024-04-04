#pragma once

#include "Coroutine/Enumerable.h"

#include <forward_list>

namespace slc {

    template<typename T,
        typename Allocator = std::allocator<T>>
        class ForwardList : public std::forward_list<T, Allocator>, public IEnumerable<T>
    {
    public:
        using std::forward_list<T, Allocator>::forward_list;

        Enumerator<T> GetEnumerator() override { return this->GetEnumeratorForRange(); }
    };

}