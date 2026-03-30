#pragma once

#include "SL/Core/Common/Reflection.h"

#include <functional>
#include <type_traits>
#include <utility>
#include <variant>

namespace sl {

	enum class EventRuntimeMode
	{
		SingleThreaded,
		MultiThreaded
	};

	enum class EventOrdering
	{
		Unordered,
		GlobalOrdered
	};

	template <
		typename TEventList,
		EventRuntimeMode Mode,
		EventOrdering Ordering >
	class BasicEventRuntime;

	template < typename TRuntime >
	class BasicEventListener;

	template < typename TEventList >
	struct EventRecord;

	template < typename TEventList >
	class EventView;

	template < typename T >
	concept IsEventList =
		requires {
			typename T::TupleType;
			typename T::VariantType;
			{ T::Size } -> std::convertible_to< size_t >;
		};

	template < typename TEventList, typename TEvent >
	concept IsRuntimeEvent =
		IsEventList< TEventList > &&
		TEventList::template Contains< TEvent >;


	namespace detail {

		template < typename Func, typename TEvent >
		concept DispatchFunctorForEvent =
			std::is_invocable_v< Func, TEvent& > &&
			( std::same_as< std::invoke_result_t< Func, TEvent& >, bool > ||
			  std::same_as< std::invoke_result_t< Func, TEvent& >, void > );

		template < typename TEventList, typename Func, size_t... Is >
		consteval bool IsDispatchFunctorForListImpl( std::index_sequence< Is... > )
		{
			return ( ... || DispatchFunctorForEvent< Func, typename TEventList::template Type< Is > > );
		}

		template < typename TEventList, typename Func >
		concept IsDispatchFunctor =
			IsEventList< TEventList > &&
			IsDispatchFunctorForListImpl< TEventList, std::remove_cvref_t< Func > >(
				std::make_index_sequence< TEventList::Size >{}
			);

		struct OrderedEventTag
		{
			explicit OrderedEventTag() = default;
		};

		inline constexpr OrderedEventTag ordered_event{};
	} // namespace detail

	template < typename TEventList >
	struct EventRecord
	{
		static_assert( IsEventList< TEventList >, "TEventList must satisfy IsEventList" );

		using EventList = TEventList;
		using Variant = typename EventList::VariantType;

		Variant data;
		bool handled = false;
		std::uint64_t sequence = 0;

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
			, sequence( seq )
		{
		}
	};

	template < typename Instance, typename MemFn >
		requires std::is_member_function_pointer_v< std::remove_cvref_t< MemFn > >
	static auto BindDispatch( Instance* instance, MemFn&& fn )
	{
		return [ instance, fn = std::forward< MemFn >( fn ) ]< typename TEvent >( TEvent& event ) -> decltype( auto )
				   requires requires { std::invoke( fn, instance, event ); }
		{
			return std::invoke( fn, instance, event );
		};
	}

	template < typename TEventList >
	class EventView
	{
		static_assert( IsEventList< TEventList >, "TEventList must satisfy IsEventList" );

	public:
		using EventList = TEventList;
		using Record = EventRecord< EventList >;

		EventView() = default;

		explicit EventView( Record& record )
			: mRecord( &record )
		{
		}

		template < typename... Funcs >
			requires( sizeof...( Funcs ) > 0 ) && ( detail::IsDispatchFunctor< EventList, Funcs > && ... )
		void Dispatch( Funcs&&... funcs )
		{
			if ( !mRecord || mRecord->handled )
				return;

			auto visitor = Overload{ std::forward< Funcs >( funcs )... };

			std::visit(
				[ & ]( auto& event ) {
					using TEvent = std::remove_cvref_t< decltype( event ) >;

					if constexpr ( std::is_invocable_v< decltype( visitor ), TEvent& > )
					{
						using Result = std::invoke_result_t< decltype( visitor ), TEvent& >;

						if constexpr ( std::is_same_v< Result, bool > )
						{
							mRecord->handled = std::invoke( visitor, event );
						}
						else
						{
							std::invoke( visitor, event );
						}
					}
				},
				mRecord->data
			);
		}

		bool IsHandled() const
		{
			return mRecord && mRecord->handled;
		}

		explicit operator bool() const
		{
			return mRecord != nullptr;
		}

	private:
		void SetHandled( bool handled = true )
		{
			if ( mRecord )
				mRecord->handled = handled;
		}

		template < typename >
		friend class BasicEventListener;

		friend class ImGuiController;

	private:
		Record* mRecord = nullptr;
	};

} // namespace sl