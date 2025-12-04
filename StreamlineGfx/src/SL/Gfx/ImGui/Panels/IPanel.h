#pragma once

#include <SL/Core/Common/Base.h>
#include <SL/Core/Events/IEventListener.h>

namespace slc {

	class IPanel : public IEventListener
	{
	public:
		virtual ~IPanel() = default;

		virtual void OnOverlayRender() = 0;

		virtual void OnEvent( Event& e ) override
		{}

		SLC_LISTENING_EVENTS( None )
	};

	template < typename T >
	concept IsPanel = DerivedFromOnly< IPanel, T >;
} // namespace Laby