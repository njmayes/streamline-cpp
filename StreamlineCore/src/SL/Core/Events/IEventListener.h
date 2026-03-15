#pragma once

#include "Event.h"

namespace sl {

	class EventRuntime;

	class IEventListener : public RefCounted
	{
	public:
		virtual ~IEventListener();

		virtual constexpr EventTypeFlag GetListeningEvents() const = 0;
		virtual void OnEvent( Event& e ) = 0;
		virtual void SetEventCondition( Predicate<>&& condition )
		{
			mAcceptCondition = std::move( condition );
		}

		bool Accept( Event& event ) const
		{
			//   Not already handled			  Correct Event Type				 Satisfies optional extra condition
			return !event.IsHandled() && ( GetListeningEvents() & event.GetType() ) && mAcceptCondition();
		}

	private:
		Predicate<> mAcceptCondition = []() { return true; };

		friend class EventRuntime;
		EventRuntime* mRuntime = nullptr;
	};

#define SL_MAKE_EVENT_FLAG( event ) ::sl::EventType::event

#define SL_LISTENING_EVENTS( ... )                                                                                  \
	static constexpr ::sl::EventTypeFlag GetStaticType()                                                            \
	{                                                                                                                \
		return ::sl::detail::BuildEventTypeMask( SL_FOR_EACH_SEP( SL_MAKE_EVENT_FLAG, SL_COMMA, __VA_ARGS__ ) ); \
	}                                                                                                                \
	virtual constexpr ::sl::EventTypeFlag GetListeningEvents() const override                                       \
	{                                                                                                                \
		return GetStaticType();                                                                                      \
	}
} // namespace sl