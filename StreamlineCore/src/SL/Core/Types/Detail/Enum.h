#pragma once

#include <SL/Core/Common/Base.h>

namespace slc::detail {

	template < auto E >
		requires std::is_scoped_enum_v< decltype( E ) >
	struct EnumTag : std::integral_constant< decltype( E ), E >
	{
		SCONSTEXPR auto Index = std::to_underlying( E );
	};

	template < auto E, typename F >
	struct EnumMatchCaseHandler
	{
		F func;

		template < typename... Args >
		auto operator()( EnumTag< E >, Args&&... args ) const
		{
			return func( std::forward< Args >( args )... );
		}
	};

	template < typename F >
	struct EnumDefaultMatchCaseHandler
	{
		F func;

		template < typename... Args >
		auto operator()( std::monostate, Args&&... args ) const
		{
			return func( std::forward< Args >( args )... );
		}
	};


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
}