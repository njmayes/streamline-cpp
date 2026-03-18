#pragma once

#include "Macros.h"
#include "Platform.h"
#include "Reflection.h"
#include "Enum.h"
#include "Environment.h"
#include "Memory.h"

#include <filesystem>
#include <format>
#include <fstream>
#include <ranges>
#include <map>
#include <queue>
#include <unordered_map>
#include <set>
#include <mutex>
#include <utility>
#include <cstddef>
#include <cstring>

namespace sl {

	using Byte = std::byte;

	template < typename TResult, typename... TArgs >
	using Func = std::function< TResult( TArgs... ) >;

	template < typename... TArgs >
	using Action = Func< void, TArgs... >;

	template < typename... T >
	using Predicate = Func< bool, T... >;


	template < Numeric T >
	struct Limits
	{
		static constexpr T Min = std::numeric_limits< T >::min();
		static constexpr T Max = std::numeric_limits< T >::max();
		static constexpr T Epsilon = std::numeric_limits< T >::epsilon();
	};

	static constexpr size_t MakeBit( int bit )
	{
		return 1ull << bit;
	}

	namespace detail {

		inline std::size_t HashCombine( std::size_t seed )
		{
			return seed;
		}

		template < typename Hash, typename T, typename... Rest >
		inline std::size_t HashCombine( std::size_t seed, const T& v, Rest... rest )
		{
			Hash hasher;
			seed ^= hasher( v ) + 0x9e3779b9 + ( seed << 6 ) + ( seed >> 2 );
			return HashCombine( seed, rest... );
		}
	} // namespace detail

	template < typename... Args >
	inline std::size_t HashCombine( Args&&... args )
	{
		return detail::HashCombine( 0, std::forward< Args >( args )... );
	}

	constexpr unsigned long long operator "" _KB( unsigned long long value )
	{
		static_assert( sizeof( unsigned long long ) >= 8, "Literal operator _KB requires at least 64-bit unsigned long long" );
		return value * 1024;
	}

	constexpr unsigned long long operator"" _MB( unsigned long long value )
	{
		return value * 1024 * 1024;
	}

	constexpr unsigned long long operator"" _GB( unsigned long long value )
	{
		return value * 1024 * 1024 * 1024;
	}
} // namespace sl