#pragma once

#include "slc/Coroutine/Enumerable.h"

#include <set>

namespace slc {

    template<typename TKey,
        typename Compare = std::less<TKey>,
        typename Allocator = std::allocator<TKey>>
    class Set : public std::set<TKey, Compare, Allocator>, public IEnumerable<TKey>
    {
    public:
        using std::set<TKey, Compare, Allocator>::set;
        using std::set<TKey, Compare, Allocator>::begin;
        using std::set<TKey, Compare, Allocator>::end;

        MAKE_RANGE_ENUMERABLE(Set)
    };

}