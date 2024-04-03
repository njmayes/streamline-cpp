#pragma once

#include "Coroutine/Enumerable.h"

#include <unordered_map>

namespace slc {

    template<typename TKey,
        typename Hash = std::hash<TKey>,
        typename KeyEqual = std::equal_to<TKey>,
        typename Allocator = std::allocator<TKey>>
        class HashMap : public std::unordered_map<TKey, Hash, KeyEqual, Allocator>, public IEnumerable<TKey>
    {
    public:
        using std::unordered_map<TKey, Hash, KeyEqual, Allocator>::unordered_multiset;
    };

}