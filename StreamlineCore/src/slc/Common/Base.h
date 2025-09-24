#pragma once

#include "Macros.h"
#include "Reflection.h"
#include "Environment.h"
#include "Memory.h"

#include <filesystem>
#include <format>
#include <fstream>
#include <ranges>
#include <map>
#include <unordered_map>
#include <utility>
#include <cstddef>
#include <cstring>

namespace slc {

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
		SCONSTEXPR T Min = std::numeric_limits< T >::min();
		SCONSTEXPR T Max = std::numeric_limits< T >::max();
		SCONSTEXPR T Epsilon = std::numeric_limits< T >::epsilon();
	};

	SCONSTEXPR size_t MakeBit( int bit )
	{
		return 1ull << bit;
	}

	inline std::size_t hash_combine( std::size_t seed )
	{
		return seed;
	}

	template < typename T, typename... Rest >
	inline void HashCombine( std::size_t& seed, const T& v, Rest... rest )
	{
		std::hash< T > hasher;
		seed ^= hasher( v ) + 0x9e3779b9 + ( seed << 6 ) + ( seed >> 2 );
		HashCombine( seed, rest... );
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
} // namespace slc