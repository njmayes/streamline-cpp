#pragma once

#include "Detail/Event.h"

#include <cstdint>
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
		ThreadOrdered,
		GlobalOrdered
	};

	template < typename TEventList, typename TEvent >
	concept IsRuntimeEvent =
		IsTypeList< TEventList > and
		TEventList::template Contains< TEvent >;

	/// <summary>
	/// Helper function for binding member functions as event handlers.
	/// </summary>
	/// <example>
	/// </example>
	template < typename MemFn >
		requires FunctionTraits< MemFn >::Valid and
				 FunctionTraits< MemFn >::IsMemberFunction and
				 ( FunctionTraits< MemFn >::Arguments::Size == 1 )
	auto BindDispatch( typename FunctionTraits< MemFn >::ObjectType* instance, MemFn&& fn )
	{
		using TArg = typename FunctionTraits< MemFn >::Arguments::template TypeAt< 0 >;

		return [ instance, fn = std::forward< MemFn >( fn ) ]( TArg arg ) -> decltype( auto ) {
			return std::invoke( fn, instance, arg );
		};
	}

	/// <summary>
	/// Wrapper type that provides a view into an event record. This is the type that is passed to event handlers when dispatching.
	/// </summary>
	template < typename TEventList >
	class EventView
	{
		SL_COMPILE_CHECK( IsTypeList< TEventList >, EventView< TEventList >, "TEventList must satisfy IsTypeList" );

		template < typename TEvent >
		static constexpr bool ValidEvent = TEventList::template Contains< std::remove_cvref_t< TEvent > >;

		using EventList = TEventList;
		using Record = detail::EventRecord< EventList >;
		using RootState = detail::RootState< TEventList >;
		using BoundState = detail::BoundState;
		using State = detail::ViewState< EventList >;

	public:
		/// <summary>
		/// Dispatch the event to a handler or set of handlers. Each handler should be an invocable object (e.g. a lambda, function pointer, or std::function)
		/// that takes either a single event argument or an EventView of a subset of the event list.
		///
		/// The dispatching will match the event against the provided handlers, and invoke those that match. If any handler returns true (or a type convertible to bool that evaluates to true), the event will be marked as handled and no further handlers will be invoked.
		/// Handlers are invoked in the order they are provided to Dispatch.
		///
		/// If the handler argument is an EventView TDispatchList > where TDispatchList is a TypeList that is a subset of EventList,
		/// the handler will be invoked if the event matches any type in TDispatchList, and the handler will receive a subview of the event with that dispatch list.
		/// </summary>
		template < typename... Funcs >
			requires( ... and detail::IsDispatchFunctor< EventList, Funcs > )
		bool Dispatch( Funcs&&... funcs )
		{
			if constexpr ( sizeof...( Funcs ) == 0 )
				return false;

			if ( mRecord.handled )
				return false;

			State old_state = mState;

			if ( auto* root = std::get_if< RootState >( &mState ) )
			{
				std::visit(
					[ & ]< typename TEvent >( TEvent& event ) {
						mState = BoundState{
							.event = &event,
							.type_tag = TypeTraits< TEvent >::BaseTag
						};
						mRecord.handled = ( ... or DispatchOne( std::forward< Funcs >( funcs ), event ) );
					},
					root->root_record->data
				);
			}
			else if ( std::holds_alternative< BoundState >( mState ) )
			{
				DispatchBoundEvent( std::forward< Funcs >( funcs )... );
			}

			mState = std::move( old_state );
			return mRecord.handled;
		}

		template < typename TEvent >
			requires ValidEvent< TEvent >
		bool Is() const
		{
			using Event = std::remove_cvref_t< TEvent >;

			if ( auto const* root = std::get_if< RootState >( &mState ) )
				return std::holds_alternative< Event >( root->root_record->data );

			if ( auto const* bound = std::get_if< BoundState >( &mState ) )
				return bound->type_tag == TypeTraits< Event >::BaseTag;

			return false;
		}

		template < IsTypeList TList >
			requires( EventList::template CompareBy< SetComparison::Subset, TList > )
		bool IsAnyOf() const
		{
			return TList::Any(
				[ this ]< typename T >() { return this->template Is< T >(); }
			);
		}

		template < typename... TEvent >
			requires( ... and ValidEvent< TEvent > )
		bool IsAnyOf() const
		{
			return ( Is< TEvent >() || ... );
		}

	private:
		explicit EventView( Record& record )
			: mRecord( record )
			, mState( RootState{ &record } )
		{
		}

		template < typename OuterEventList >
			requires( EventList::template CompareBy< SetComparison::Subset, OuterEventList > )
		explicit EventView( EventView< OuterEventList > const& subview )
			: mRecord( subview.mRecord )
		{
			if ( auto const* bound = std::get_if< typename EventView< OuterEventList >::BoundState >( &subview.mState ) )
			{
				mState = BoundState{
					.event = bound->event,
					.type_tag = bound->type_tag
				};
			}
			else
			{
				mState = std::monostate{};
			}
		}

		template < typename... Funcs >
		void DispatchBoundEvent( Funcs&&... funcs )
		{
			auto* bound = std::get_if< BoundState >( &mState );
			SL_VERIFY( bound );
			SL_VERIFY( bound->event );
			SL_VERIFY( bound->type_tag );

			EventList::Any(
				[ & ]< typename TEvent >() {
					if ( bound->type_tag != TypeTraits< TEvent >::BaseTag )
						return false;

					if constexpr ( detail::AnyDispatchFunctorMatchesEvent< TEvent, EventList, Funcs... > )
					{
						auto& event = *static_cast< TEvent* >( bound->event );
						mRecord.handled = ( ... or DispatchOne( std::forward< Funcs >( funcs ), event ) );
					}

					return true;
				}
			);
		}

		template < typename Func, typename Arg >
		bool InvokeDispatchHandler( Func&& func, Arg&& arg )
		{
			using Result = typename FunctionTraits< std::remove_cvref_t< Func > >::ReturnType;

			if constexpr ( std::convertible_to< Result, bool > )
			{
				return static_cast< bool >( std::invoke( std::forward< Func >( func ), std::forward< Arg >( arg ) ) );
			}
			else
			{
				std::invoke( std::forward< Func >( func ), std::forward< Arg >( arg ) );
				return false;
			}
		}

		template < typename Func, typename TEvent >
		bool DispatchOne( Func&& func, TEvent& event )
		{
			using TFunc = std::remove_cvref_t< Func >;
			using Event = std::remove_cvref_t< TEvent >;

			if constexpr ( detail::SingleEventDispatchFunctor< TFunc, EventList > )
			{
				return DispatchSingle( std::forward< Func >( func ), event );
			}
			else if constexpr ( detail::TypeListEventDispatchFunctor< TFunc, EventList > )
			{
				return DispatchTypeList( std::forward< Func >( func ), event );
			}
		}

		template < typename Func, typename Event >
		bool DispatchSingle( Func&& func, Event& event )
		{
			using TFunc = std::remove_cvref_t< Func >;
			using TEvent = std::remove_cvref_t< Event >;

			if constexpr ( std::same_as< detail::DispatchEventType< TFunc >, TEvent > )
			{
				return InvokeDispatchHandler( std::forward< Func >( func ), event );
			}

			return false;
		}

		template < typename Func, typename Event >
		bool DispatchTypeList( Func&& func, Event& )
		{
			using TFunc = std::remove_cvref_t< Func >;
			using TEvent = std::remove_cvref_t< Event >;
			using TDispatchList = detail::TypeListDispatchList< TFunc >;

			if constexpr ( TDispatchList::template Contains< TEvent > )
			{
				EventView< TDispatchList > subview( *this );
				return InvokeDispatchHandler( std::forward< Func >( func ), subview );
			}

			return false;
		}

	private:
		detail::EventRecordBase& mRecord = nullptr;
		State mState{};

		template < typename TOtherList >
		friend class EventView;

		template <
			typename TList,
			EventRuntimeMode Mode,
			EventOrdering Ordering >
		friend class BasicEventRuntime;
	};

} // namespace sl