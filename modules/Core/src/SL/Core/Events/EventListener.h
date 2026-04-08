#pragma once

#include "EventDevice.h"

namespace sl {

	template < typename TRuntime >
	class BasicEventListener : public virtual BasicEventDevice< TRuntime >
	{
	public:
		using Event = typename BasicEventDevice< TRuntime >::Event;

		void SetEventCondition( Predicate< Event& > condition )
		{
			mAcceptCondition = std::move( condition );
		}

		virtual bool OnEvent( Event& )
		{
			return false;
		};

		bool Accept( Event& e ) const override
		{
			return ShouldAcceptEvent( e ) and mAcceptCondition( e );
		}

	private:
		// Events are ignored by default - listeners must explicitly specify which events they want to receive
		// by overriding this function or using the SL_LISTENING_EVENTS macros
		virtual bool ShouldAcceptEvent( Event& ) const
		{
			return false;
		}

	private:
		Predicate< Event& > mAcceptCondition = []( auto&& ) { return true; };
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