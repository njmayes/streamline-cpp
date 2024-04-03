#pragma once

#include "Coroutine/Enumerable.h"

#include <array>

namespace slc {

    template<typename T, size_t TSize>
    class Array : public std::array<T, TSize>, public IEnumerable<T>
    {
    public:
        using std::array<T, TSize>::array;

        Array() { memset(this->data(), 0, TSize); }
    };

}