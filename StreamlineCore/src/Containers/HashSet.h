#pragma once

#include "Coroutine/Enumerable.h"

#include <unordered_set>

namespace slc {

    template<typename TKey,
        typename Hash = std::hash<TKey>,
        typename KeyEqual = std::equal_to<TKey>,
        typename Allocator = std::allocator<TKey>>
        class HashSet : public std::unordered_set<TKey, Hash, KeyEqual, Allocator>, public IEnumerable<TKey>
    {
    public:
        using std::unordered_set<TKey, Hash, KeyEqual, Allocator>::unordered_set;
    };

}