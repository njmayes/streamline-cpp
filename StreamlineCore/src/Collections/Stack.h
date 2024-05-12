#pragma once

#include "Coroutine/Enumerable.h"

#include <stack>

namespace slc {

    template<typename T,
        typename Container = std::deque<T>>
    class Stack : public std::stack<T, Container>, public IEnumerable<T>
    {
    public:
        using std::stack<T, Container>::stack;
        using std::stack<T, Container>::begin;
        using std::stack<T, Container>::end;

        MAKE_RANGE_ENUMERABLE(Stack)
    };

}