#pragma once

#include <SL/Core/Common/Base.h>

namespace sl::detail {

	template < auto E >
		requires std::is_enum_v< decltype( E ) >
	struct EnumTag : std::integral_constant< decltype( E ), E >
	{
		static consteval std::size_t ComputeIndex()
		{
			constexpr auto maybe_index = magic_enum::enum_index( E );
			static_assert( maybe_index.has_value(), "EnumTag: value not reflectable by magic_enum" );
			return static_cast< std::size_t >( *maybe_index );
		}

		static constexpr std::size_t Index = ComputeIndex();
	};

	template < auto E, typename F >
	struct EnumMatchCaseHandler
	{
		F func;
		static constexpr auto Element = E;

		template < typename... Args >
			requires std::invocable< F const&, Args... >
		decltype( auto ) operator()( EnumTag< E >, Args&&... args ) const
		{
			return func( std::forward< Args >( args )... );
		}
	};

	template < typename F >
	struct EnumDefaultMatchCaseHandler
	{
		F func;

		decltype( auto ) operator()( std::monostate ) const
			requires std::invocable< F const& >
		{
			return func();
		}
	};

	template< typename T >
	struct IsEnumMatchCaseHandler : std::false_type
	{};

	template< auto E, typename F >
	struct IsEnumMatchCaseHandler< EnumMatchCaseHandler< E, F > > : std::true_type
	{};

	template < typename... Fs >
	struct Overload : Fs...
	{
		using Fs::operator()...;
	};


	template < typename T >
	struct IsDefaultMatchCaseHandler : std::false_type
	{};

	template < typename F >
	struct IsDefaultMatchCaseHandler< EnumDefaultMatchCaseHandler< F > > : std::true_type
	{};

	template < typename... Ts >
	struct FindDefaultHandlerHelper
	{
		using type = void; // Sentinel indicating "not found"
	};

	template < typename T, typename... Rest >
	struct FindDefaultHandlerHelper< T, Rest... >
	{
		using type = std::conditional_t<
			IsDefaultMatchCaseHandler< T >::value,
			T,
			typename FindDefaultHandlerHelper< Rest... >::type >;
	};


	template < typename T >
	struct ExtractDefaultHandler;

	template < template < typename... > class OverloadT, typename... Cases >
	struct ExtractDefaultHandler< OverloadT< Cases... > >
	{
		using type = typename FindDefaultHandlerHelper< Cases... >::type;
	};
} // namespace sl::detail