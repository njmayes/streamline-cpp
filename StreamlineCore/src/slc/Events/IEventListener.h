#pragma once

#include "EventRuntime.h"

namespace slc {

	class IEventListener
	{
	public:
		IEventListener()
		{
			EventRuntime::RegisterListener( this );
		}
		virtual ~IEventListener()
		{
			EventRuntime::DeregisterListener( this );
		}

		virtual constexpr EventTypeFlag GetListeningEvents() const = 0;
		virtual void OnEvent( Event& e ) = 0;
		virtual void SetEventCondition( Predicate<>&& condition )
		{
			mAcceptCondition = std::move( condition );
		}

		bool Accept( Event& event ) const
		{
			//   Not already handled				Valid Event Type				 Satisfies optional extra condition
			return !event.IsHandled() && ( GetListeningEvents() & event.GetType() ) && mAcceptCondition();
		}

	private:
		Predicate<> mAcceptCondition = []() { return true; };
	};

#define SLC_MAKE_EVENT_FLAG( event ) ::slc::EventType::event

#define SLC_LISTENING_EVENTS( ... )                                                                                  \
	static constexpr ::slc::EventTypeFlag GetStaticType()                                                            \
	{                                                                                                                \
		return ::slc::detail::BuildEventTypeMask( SLC_FOR_EACH_SEP( SLC_MAKE_EVENT_FLAG, SLC_COMMA, __VA_ARGS__ ) ); \
	}                                                                                                                \
	virtual constexpr ::slc::EventTypeFlag GetListeningEvents() const override                                       \
	{                                                                                                                \
		return GetStaticType();                                                                                      \
	}
} // namespace slc