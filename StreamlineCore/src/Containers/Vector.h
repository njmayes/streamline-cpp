#pragma once

#include "Coroutine/Enumerable.h"

#include <vector>

namespace slc {

    template<typename T,
        typename Allocator = std::allocator<T>>
    class Vector : public std::vector<T, Allocator>, public IEnumerable<T>
    {
    public:
        using std::vector<T, Allocator>::vector;
    };

}