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
			std::same_as< std::remove_cvref_t< R >, void > or
			std::convertible_to< R, bool >;



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
			std::remove_cvref_t< typename DispatchArguments< Func >::template TypeAt< 0 > >;

		template < typename Func >
		using TypeListDispatchList = EventViewEventList< DispatchEventType< Func > >;


		template < typename Func, typename TEventList >
		concept SingleEventDispatchFunctor = TEventList::template Contains< DispatchEventType< Func > >; // The argument type must be in the event list


		template < typename Func, typename TEventList >
		concept TypeListEventDispatchFunctor =
			IsEventViewV< DispatchEventType< Func > > and										   // The argument must satisfy IsEventView
			IsTypeList< TypeListDispatchList< Func > > and										   // The EventView must wrap a type list
			TEventList::template CompareBy< SetComparison::Subset, TypeListDispatchList< Func > >; // The wrapped type list must be a subset of the event list


		template < typename TEventList, typename Func >
		concept IsDispatchFunctor =
			IsTypeList< TEventList > and																			// TEventList must satisfy IsTypeList
			DispatchFunctionTraits< Func >::Valid and																// Must be a valid function type
			ValidDispatchReturn< DispatchReturnType< Func > > and													// Return type must be void or convertible to bool
			( DispatchArguments< Func >::Size == 1 ) and															// Must take exactly one argument
			( SingleEventDispatchFunctor< Func, TEventList > or TypeListEventDispatchFunctor< Func, TEventList > ); // Must satisfy either dispatch functor concept (see above)


		template < typename Func, typename TEvent, typename TEventList >
		concept DispatchFunctorMatchesEvent =
			IsDispatchFunctor< TEventList, Func > and // Func must satisfy IsDispatchFunctor for the event list
			(
				// If it's a single event dispatch functor, the event type must match exactly
				( SingleEventDispatchFunctor< Func, TEventList > and std::same_as< DispatchEventType< Func >, std::remove_cvref_t< TEvent > > ) or
				// If it's a type list dispatch functor, the event type must be contained in the dispatch list
				( TypeListEventDispatchFunctor< Func, TEventList > and TypeListDispatchList< Func >::template Contains< std::remove_cvref_t< TEvent > > )
			);

		template < typename TEvent, typename TEventList, typename... Funcs >
		concept AnyDispatchFunctorMatchesEvent =
			( DispatchFunctorMatchesEvent< Funcs, TEvent, TEventList > || ... );


		struct OrderedEventTag
		{
			explicit OrderedEventTag() = default;
		};

		inline constexpr OrderedEventTag ordered_event{};


		struct EventRecordBase
		{
			bool handled = false;
			std::uint64_t sequence = 0;
		};

		template < typename TEventList >
		struct EventRecord : EventRecordBase
		{
			static_assert( IsTypeList< TEventList >, "TEventList must satisfy IsTypeList" );

			using EventList = TEventList;
			using Variant = typename EventList::VariantType;

			Variant data;

			EventRecord() = default;

			template < typename TEvent, typename... Args >
				requires IsRuntimeEvent< TEventList, TEvent > && std::constructible_from< TEvent, Args... >
			explicit EventRecord( std::in_place_type_t< TEvent >, Args&&... args )
				: data( std::in_place_type< TEvent >, std::forward< Args >( args )... )
			{
			}

			template < typename TEvent, typename... Args >
				requires IsRuntimeEvent< TEventList, TEvent > && std::constructible_from< TEvent, Args... >
			explicit EventRecord( std::in_place_type_t< TEvent >, detail::OrderedEventTag, std::uint64_t seq, Args&&... args )
				: data( std::in_place_type< TEvent >, std::forward< Args >( args )... )
			{
				this->sequence = seq;
			}
		};

		template < typename TEventList >
		struct RootState
		{
			EventRecord< TEventList >* root_record = nullptr;
		};

		struct BoundState
		{
			void* event{};
			TypeTag type_tag{};
		};

		template < typename TEventList >
		using ViewState = std::variant< std::monostate, RootState< TEventList >, BoundState >;
	} // namespace detail
} // namespace sl