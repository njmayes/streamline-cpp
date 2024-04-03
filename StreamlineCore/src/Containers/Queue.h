#pragma once

#include "Coroutine/Enumerable.h"

#include <queue>

namespace slc {

    template<typename T,
        typename Container = std::deque<T>>
        class Queue : public std::queue<T, Container>, public IEnumerable<T>
    {
    public:
        using std::queue<T, Container>::queue;
    };

}
