#pragma once

#include "SL/Core/Common/Base.h"

#include <functional>
#include <type_traits>
#include <utility>
#include <variant>

namespace sl {

	template < typename TEventList >
	class EventView;

	namespace detail {

		// Dispatch Return Check
		template < typename R >
		concept ValidDispatchReturn =
			std::same_as< std::remove_cvref_t< R >, void > ||
			std::convertible_to< R, bool >;


		// Type List Subset Check

		template < typename TSubset, typename TSuperset >
		struct IsTypeListSubset : std::false_type
		{};

		template < typename... SubsetTs, typename TSuperset >
			requires IsTypeList< TSuperset >
		struct IsTypeListSubset< RawTypeList< SubsetTs... >, TSuperset >
			: std::bool_constant< ( TSuperset::template Contains< std::remove_cvref_t< SubsetTs > > && ... ) >
		{};

		template < typename... SubsetTs, typename TSuperset >
			requires IsTypeList< TSuperset >
		struct IsTypeListSubset< TypeList< SubsetTs... >, TSuperset >
			: std::bool_constant< ( TSuperset::template Contains< std::remove_cvref_t< SubsetTs > > && ... ) >
		{};

		template < typename TSubset, typename TSuperset >
		inline constexpr bool IsTypeListSubsetV =
			IsTypeListSubset< std::remove_cvref_t< TSubset >, std::remove_cvref_t< TSuperset > >::value;


		// EventView Check

		template < typename T >
		struct EventViewTraits
		{
			static constexpr bool Valid = false;
		};

		template < typename T >
		struct EventViewTraits< EventView< T > >
		{
			static constexpr bool Valid = true;
			using EventList = T;
		};

		template < typename T >
		inline constexpr bool IsEventViewV =
			EventViewTraits< std::remove_cvref_t< T > >::Valid;

		template < typename T >
		using EventViewEventList = typename EventViewTraits< std::remove_cvref_t< T > >::EventList;


		// Dispatch Functor Helpers

		template < typename Func >
		using DispatchFunctionTraits = FunctionTraits< std::remove_cvref_t< Func > >;

		template < typename Func >
		using DispatchArguments = typename DispatchFunctionTraits< Func >::Arguments;

		template < typename Func >
		using DispatchReturnType = typename DispatchFunctionTraits< Func >::ReturnType;

		template < typename Func >
			requires( DispatchArguments< Func >::Size == 1 )
		using DispatchEventType =
			std::remove_cvref_t< typename DispatchArguments< Func >::template Type< 0 > >;

		template < typename Func >
		using TypeListDispatchList = EventViewEventList< DispatchEventType< Func > >;


		template < typename Func, typename TEventList >
		concept SingleEventDispatchFunctor = TEventList::template Contains< DispatchEventType< Func > >; // The argument type must be in the event list


		template < typename Func, typename TEventList >
		concept TypeListEventDispatchFunctor =
			IsEventViewV< DispatchEventType< Func > > &&				   // The argument must satisfy IsEventView
			IsTypeList< TypeListDispatchList< Func > > &&				   // The EventView must wrap a type list
			IsTypeListSubsetV< TypeListDispatchList< Func >, TEventList >; // The wrapped type list must be a subset of the event list


		template < typename TEventList, typename Func >
		concept IsDispatchFunctor =
			IsTypeList< TEventList > &&																				// TEventList must satisfy IsTypeList
			DispatchFunctionTraits< Func >::Valid &&																// Must be a valid function type
			ValidDispatchReturn< DispatchReturnType< Func > > &&													// Return type must be void or convertible to bool
			( DispatchArguments< Func >::Size == 1 ) &&																// Must take exactly one argument
			( SingleEventDispatchFunctor< Func, TEventList > || TypeListEventDispatchFunctor< Func, TEventList > ); // Must satisfy either dispatch functor concept (see above)


		struct OrderedEventTag
		{
			explicit OrderedEventTag() = default;
		};

		inline constexpr OrderedEventTag ordered_event{};

	} // namespace detail
} // namespace sl