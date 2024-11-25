#pragma once

#include <slc/Collections/StaticMap.h>

namespace slc {

    template<typename T, Numeric KeyType = int>
    struct TableValue
    {
        int low, high;
        T value;
    };

    template<typename T, std::size_t N, Numeric KeyType = int>
    class RangeTable
    {
    public:
        using ElementType = TableValue<T, KeyType>;
        using MapValueType = detail::MapElement<KeyType, ElementType>;

        template<std::size_t... I>
        constexpr RangeTable(const ElementType(&data)[N]) noexcept
            : mTable(MakePairs(data, std::make_index_sequence<N>()))
        {}

        constexpr std::optional<T> TryLookup(KeyType value) const noexcept
        {
            auto it = mTable.upper_bound(value);
            if (it != mTable.begin())
            {
                --it;
                if (value >= it->second.low and value < it->second.high)
                {
                    return it->second.value;
                }
            }

            return std::nullopt;
        }

        constexpr bool Contains(KeyType value) const noexcept
        {
            auto it = mTable.upper_bound(value);
            if (it == mTable.begin())
                return false;

            it--;
            return value >= it->second.low and value < it->second.high;
        }

    private:
        template<std::size_t... I>
        constexpr std::array<std::pair<KeyType, ElementType>, sizeof...(I)> MakePairs(const ElementType(&data)[N], std::index_sequence<I...>)
        {
            return { { MakePair<I>(data[I])... } };
        }

        template<std::size_t I>
        constexpr std::pair<KeyType, ElementType> MakePair(const ElementType& data)
        {
            return std::make_pair(data.low, data);
        }

    private:
        slc::StaticMap<MapValueType, N> mTable;
    };

    template<typename T, std::size_t N, Numeric KeyType = int>
    static consteval RangeTable<T, N, KeyType> MakeRangeTable(const TableValue<T, KeyType>(&items)[N])
    {
        return RangeTable<T, N, KeyType>(items);
    }
}