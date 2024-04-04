#pragma once

#include "Coroutine/Enumerable.h"

#include <set>

namespace slc {

    template<typename TKey,
        typename Compare = std::less<TKey>,
        typename Allocator = std::allocator<TKey>>
        class Set : public std::set<TKey, Compare, Allocator>, public IEnumerable<TKey>
    {
    public:
        using std::set<TKey, Compare, Allocator>::set;

        MAKE_RANGE_ENUMERABLE(Set)
    };

}