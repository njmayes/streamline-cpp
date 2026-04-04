#pragma once

#include "Context.h"
#include "Connection.h"

#include "SL/Core/Common/Application.h"

namespace sl::net {

	class NetLayer : public ApplicationLayer
	{
	public:
		SL_LISTENING_EVENTS( NetworkOutEvent );

		template < ContextOptionsType options_t >
		NetLayer( options_t const& options )
			: mContext( options )
		{
		}

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