#pragma once

#include "Coroutine/Enumerable.h"

#include <span>

namespace slc {

    template<typename T,
        size_t Extent = std::dynamic_extent>
    class Span : public std::span<T, Extent>, public IEnumerable<T>
    {
    public:
        using std::span<T, Extent>::span;
    };
}