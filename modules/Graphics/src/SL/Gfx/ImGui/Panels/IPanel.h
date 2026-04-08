#pragma once

#include <SL/Core/Common/Application.h>
#include <SL/Core/Logging/Log.h>

namespace sl {

	class IPanel : public ApplicationEventListener, public ApplicationEventEmitter
	{
	public:
		virtual ~IPanel() = default;

		virtual void OnOverlayRender() = 0;
	};

	template < typename T >
	concept IsPanel = DerivedFromOnly< T, IPanel >;
} // namespace sl