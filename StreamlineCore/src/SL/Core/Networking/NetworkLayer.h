#pragma once

#include "Context.h"
#include "Connection.h"

#include "SL/Core/Common/Application.h"
#include "SL/Core/Events/EventTypes.h"

namespace sl::net {

	class NetLayer : public ApplicationLayer
	{
	public:
		SLC_LISTENING_EVENTS( NetworkOut );

		template < ContextOptionsType options_t >
		NetLayer( options_t const& options )
			: mContext( options )
		{
		}

		void OnUpdate( Timestep )
		{}
		void OnRender() override
		{}
		void OnOverlayRender() override
		{}

		virtual void OnConnect( ConnectionPtr )
		{}
		virtual void OnDisconnect( ConnectionPtr )
		{}

		virtual void OnMessage( Payload const& )
		{}

	protected:
		Context mContext;
	};
} // namespace sl::net