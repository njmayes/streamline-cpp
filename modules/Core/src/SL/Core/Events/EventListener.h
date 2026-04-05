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

		virtual ~BasicEventListener()
		{
			if ( mRuntime )
				mRuntime->DeregisterListener( this );
		}

		virtual bool OnEvent( Event& )
		{
			return false;
		};

		virtual void SetEventCondition( Predicate<> condition )
		{
			mAcceptCondition = std::move( condition );
		}

		bool Accept( Event& e ) const
		{
			return ShouldAcceptEvent( e ) and mAcceptCondition();
		}

	private:
		// Events are ignored by default - listeners must explicitly specify which events they want to receive
		// by overriding this function or using the SL_LISTENING_EVENTS macros
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

#define SL_LISTENING_EVENTS( ... )                          \
	using ListeningEvents = ::sl::TypeList< __VA_ARGS__ >;  \
	bool ShouldAcceptEvent( Event& event ) const override   \
	{                                                       \
		return event.template IsAnyOf< ListeningEvents >(); \
	}

#define SL_LISTENING_EVENTS_DERIVED( BaseType, ... )       \
	using ListeningEvents = ::sl::TypeList< __VA_ARGS__ >; \
	bool ShouldAcceptEvent( Event& event ) const override  \
	{                                                      \
		return event.IsAnyOf< ListeningEvents >() ||       \
			   BaseType::ShouldAcceptEvent( event );       \
	}

} // namespace sl