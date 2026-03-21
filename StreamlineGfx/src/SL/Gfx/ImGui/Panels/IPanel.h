#pragma once

#include <SL/Core/Events/IEventListener.h>
#include <SL/Core/Logging/Log.h>

namespace sl {

	class IPanel : public IEventListener
	{
	public:
		virtual ~IPanel() = default;

		virtual void OnOverlayRender() = 0;

		virtual void OnEvent( Event& e ) override
		{}

		SL_LISTENING_EVENTS( None )
	};

	template < typename T >
	concept IsPanel = DerivedFromOnly< IPanel, T >;
} // namespace sl