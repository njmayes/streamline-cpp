#pragma once

#include "EventList.h"

namespace sl {

	template <
		typename TEventList,
		EventRuntimeMode Mode,
		EventOrdering Ordering >
	class BasicEventRuntime;

	template < typename TRuntime >
	class BasicEventListener : public RefCounted
	{
	public:
		using Runtime = TRuntime;
		using EventList = typename Runtime::EventList;
		using Event = typename Runtime::EventType;

		static_assert( IsEventList< EventList >, "Runtime::EventList must satisfy IsEventList" );

		using ListeningEvents = TypeList<>;

		virtual ~BasicEventListener()
		{
			if ( mRuntime )
				mRuntime->DeregisterListener( this );
		}

		virtual void OnEvent( Event& e ) = 0;

		virtual void SetEventCondition( Predicate<>&& condition )
		{
			mAcceptCondition = std::move( condition );
		}

		bool Accept( Event& event ) const
		{
			if ( event.IsHandled() )
				return false;

			bool accepts = false;

			std::visit(
				[ & ]( auto& concrete_event ) {
					using TEvent = std::remove_cvref_t< decltype( concrete_event ) >;
					accepts = ListeningEvents::template Contains< TEvent >;
				},
				event.mRecord->data
			);

			return accepts and mAcceptCondition();
		}

	private:
		Predicate<> mAcceptCondition = []() { return true; };

		template < typename, EventRuntimeMode, EventOrdering >
		friend class BasicEventRuntime;

		Runtime* mRuntime = nullptr;
	};

#define SL_LISTENING_EVENTS( ... )                                 \
private:                                                           \
	using DeclaredListeningEvents = ::sl::TypeList< __VA_ARGS__ >; \
                                                                   \
public:                                                            \
	using ListeningEvents = DeclaredListeningEvents;

} // namespace sl