#pragma once

#include <slc/Common/Base.h>
#include <slc/Events/IEventListener.h>

namespace slc {

	class IPanel : public IEventListener, public RefCounted
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