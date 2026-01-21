#include "ClientLayer.h"

#include "SL/Core/Common/Application.h"
#include "SL/Core/Logging/Log.h"

namespace sl::net {

	ClientLayer::ClientLayer( ClientContextOptions const& opts )
		: NetLayer( opts )
		, mOptions( opts )
	{
	}

	void ClientLayer::OnAttach()
	{
		Connect( mOptions.host, mOptions.port );

		mContext.Run();
	}

	void ClientLayer::OnDetach()
	{
		mContext.Stop();
	}

	void ClientLayer::OnEvent( Event& e )
	{
		e.Dispatch< NetworkOutEvent >( SLC_BIND_EVENT_FUNC( SendMessage ) );
	}

	void ClientLayer::Connect( std::string const& host, std::uint16_t port )
	{
		mContext.Connect( host, port, [ this, host, port ]( sl::net::ConnectionPtr connection ) {
			sl::log::Info( "Client connected to {}:{}", host, port );

			mServerConnection = connection;

			mServerConnection->OnConnect( [ this, connection ] {
				OnConnect( connection );
			} );
			mServerConnection->OnDisconnect( [ this, connection ] { OnDisconnect( connection ); } );

			mServerConnection->OnRead( [ this ]( sl::net::Payload const& msg ) {
				OnMessage( msg );
				Application::PostEvent< NetworkInEvent >( msg );
			} );

			mServerConnection->Start();
		} );
	}

	bool ClientLayer::SendMessage( NetworkOutEvent& e )
	{
		mServerConnection->AddToQueue( e.data );
		return false;
	}
} // namespace sl::net