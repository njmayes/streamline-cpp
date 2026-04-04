#pragma once

#include "Detail/Event.h"

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

	template < typename TEventList, typename TEvent >
	concept IsRuntimeEvent =
		IsTypeList< TEventList > &&
		TEventList::template Contains< TEvent >;

	namespace detail {

		template < typename T >
		inline constexpr int EventTypeTagValue = 0;

		template < typename T >
		constexpr void const* EventTypeTag() noexcept
		{
			return &EventTypeTagValue< std::remove_cvref_t< T > >;
		}

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

	} // namespace detail

	template < typename MemFn >
		requires FunctionTraits< MemFn >::Valid &&
				 FunctionTraits< MemFn >::IsMemberFunction &&
				 ( FunctionTraits< MemFn >::Arguments::Size == 1 )
	auto BindDispatch( typename FunctionTraits< MemFn >::ObjectType* instance, MemFn&& fn )
	{
		using TArg = typename FunctionTraits< MemFn >::Arguments::template Type< 0 >;

		return [ instance, fn = std::forward< MemFn >( fn ) ]( TArg arg ) -> decltype( auto ) {
			return std::invoke( fn, instance, arg );
		};
	}

	template < typename TEventList >
	class EventView
	{
		static_assert( IsTypeList< TEventList >, "TEventList must satisfy IsTypeList" );

	public:
		using EventList = TEventList;
		using Record = detail::EventRecord< EventList >;

		EventView() = default;
		EventView( EventView const& ) = default;
		EventView& operator=( EventView const& ) = default;

		explicit EventView( Record& record )
			: mRecord( &record )
			, mRootRecord( &record )
		{
		}

		template < typename TEventCheck >
		bool IsType() const
		{
			if ( !mRecord )
				return false;

			if constexpr ( IsTypeList< TEventCheck > )
			{
				if ( mEvent )
				{
					return TEventCheck::Any(
						[ this ]< typename T >() {
							return this->template Is< T >();
						}
					);
				}
				else
				{
					if ( !mRootRecord )
						return false;

					return TEventCheck::Any(
						[ this ]< typename T >() {
							return std::holds_alternative< T >( RootRecord().data );
						}
					);
				}
			}
			else
			{
				if ( mEvent )
				{
					return this->template Is< TEventCheck >();
				}
				else
				{
					if ( !mRootRecord )
						return false;

					return std::holds_alternative< TEventCheck >( RootRecord().data );
				}
			}
		}

		template < typename TEvent >
			requires EventList::template
		Contains< std::remove_cvref_t< TEvent > > bool Is() const
		{
			return mTypeTag == detail::EventTypeTag< std::remove_cvref_t< TEvent > >();
		}

		template < typename TEvent >
			requires EventList::template
		Contains< std::remove_cvref_t< TEvent > >
			std::remove_cvref_t< TEvent >& Get() &
		{
			SL_ASSERT( mEvent );
			SL_ASSERT( this->template Is< TEvent >() );
			return *static_cast< std::remove_cvref_t< TEvent >* >( mEvent );
		}

		template < typename TEvent >
			requires EventList::template
		Contains< std::remove_cvref_t< TEvent > >
			std::remove_cvref_t< TEvent > const& Get() const&
		{
			SL_ASSERT( mEvent );
			SL_ASSERT( this->template Is< TEvent >() );
			return *static_cast< std::remove_cvref_t< TEvent > const* >( mEvent );
		}

		template < typename... Funcs >
			requires( sizeof...( Funcs ) > 0 ) && ( ... && detail::IsDispatchFunctor< EventList, Funcs > )
		void Dispatch( Funcs&&... funcs )
		{
			if ( !mRecord || mRecord->handled )
				return;

			SL_ASSERT( mRootRecord );

			void* old_event = mEvent;
			void const* old_type_tag = mTypeTag;

			std::visit(
				[ & ]< typename TEvent >( TEvent& event ) {
					mEvent = &event;
					mTypeTag = detail::EventTypeTag< TEvent >();
					( DispatchOne( std::forward< Funcs >( funcs ), event ), ... );
				},
				RootRecord().data
			);

			mEvent = old_event;
			mTypeTag = old_type_tag;
		}

		bool IsHandled() const
		{
			return mRecord && mRecord->handled;
		}

		std::uint64_t Sequence() const
		{
			return mRecord ? mRecord->sequence : 0;
		}

		explicit operator bool() const
		{
			return mRecord != nullptr;
		}

	private:
		template < typename OuterEventList >
			requires detail::IsTypeListSubsetV< EventList, OuterEventList >
		explicit EventView( EventView< OuterEventList > const& subview )
			: mRecord( subview.mRecord )
			, mRootRecord( nullptr )
			, mEvent( subview.mEvent )
			, mTypeTag( subview.mTypeTag )
		{
		}

		Record& RootRecord() const
		{
			return *mRootRecord;
		}

		template < typename Func, typename Arg >
		void InvokeDispatchHandler( Func&& func, Arg&& arg )
		{
			using Result = typename FunctionTraits< std::remove_cvref_t< Func > >::ReturnType;

			if constexpr ( std::convertible_to< Result, bool > )
			{
				mRecord->handled = static_cast< bool >( std::invoke( std::forward< Func >( func ), std::forward< Arg >( arg ) ) );
			}
			else
			{
				std::invoke( std::forward< Func >( func ), std::forward< Arg >( arg ) );
			}
		}

		template < typename Func, typename TEvent >
		void DispatchOne( Func&& func, TEvent& event )
		{
			if ( !mRecord || mRecord->handled )
				return;

			using TFunc = std::remove_cvref_t< Func >;
			using Event = std::remove_cvref_t< TEvent >;

			if constexpr ( detail::SingleEventDispatchFunctor< TFunc, EventList > )
			{
				DispatchSingle( std::forward< Func >( func ), event );
			}
			else if constexpr ( detail::TypeListEventDispatchFunctor< TFunc, EventList > )
			{
				DispatchTypeList( std::forward< Func >( func ), event );
			}
		}

		template < typename Func, typename Event >
		void DispatchSingle( Func&& func, Event& event )
		{
			using TFunc = std::remove_cvref_t< Func >;
			using TEvent = std::remove_cvref_t< Event >;

			if constexpr ( std::same_as< detail::DispatchEventType< TFunc >, TEvent > )
			{
				InvokeDispatchHandler( std::forward< Func >( func ), event );
			}
		}

		template < typename Func, typename Event >
		void DispatchTypeList( Func&& func, Event& )
		{
			using TFunc = std::remove_cvref_t< Func >;
			using TEvent = std::remove_cvref_t< Event >;
			using TDispatchList = detail::TypeListDispatchList< TFunc >;

			if constexpr ( TDispatchList::template Contains< TEvent > )
			{
				EventView< TDispatchList > subview( *this );
				InvokeDispatchHandler( std::forward< Func >( func ), subview );
			}
		}

	private:
		detail::EventRecordBase* mRecord = nullptr;
		Record* mRootRecord = nullptr;
		void* mEvent = nullptr;
		void const* mTypeTag = nullptr;

		template < typename TOtherList >
		friend class EventView;
	};

} // namespace sl