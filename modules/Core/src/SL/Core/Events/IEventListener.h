#pragma once

#include "Event.h"

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

		static_assert( IsTypeList< EventList >, "Runtime::EventList must satisfy IsTypeList" );

		using ListeningEvents = TypeList<>;

		virtual ~BasicEventListener()
		{
			if ( mRuntime )
				mRuntime->DeregisterListener( this );
		}

		virtual void OnEvent( Event& e ) {};

		virtual void SetEventCondition( Predicate<> condition )
		{
			mAcceptCondition = std::move( condition );
		}

		bool Accept( Event& e ) const
		{
			return not e.IsHandled() and ShouldAcceptEvent( e ) and mAcceptCondition();
		}

	private:
		// Accept all events if not overridden, otherwise the listener will only accept events of the types specified in SL_LISTENING_EVENTS.
		virtual bool ShouldAcceptEvent( Event& ) const
		{
			return false;
		}

	private:
		Predicate<> mAcceptCondition = []() { return true; };

		template < typename, EventRuntimeMode, EventOrdering >
		friend class BasicEventRuntime;

		Runtime* mRuntime = nullptr;
	};

#define SL_LISTENING_EVENTS( ... )                         \
	using ListeningEvents = ::sl::TypeList< __VA_ARGS__ >; \
	bool ShouldAcceptEvent( Event& event ) const override  \
	{                                                      \
		return event.template IsType< ListeningEvents >(); \
	}

#define SL_LISTENING_EVENTS_DERIVED( BaseType, ... )       \
	using ListeningEvents = ::sl::TypeList< __VA_ARGS__ >; \
	bool ShouldAcceptEvent( Event& event ) const override  \
	{                                                      \
		return event.IsType< ListeningEvents >() ||        \
			   BaseType::ShouldAcceptEvent( event );       \
	}

} // namespace sl