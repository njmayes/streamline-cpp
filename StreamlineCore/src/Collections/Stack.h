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

        MAKE_RANGE_ENUMERABLE(Stack)
    };

}