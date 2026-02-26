#include "ServerLayer.h"

#include "SL/Core/Common/Application.h"
#include "SL/Core/Logging/Log.h"

namespace sl::net {

	ServerLayer::ServerLayer( ServerContextOptions const& opts )
		: NetLayer( opts )
		, mOptions( opts )
	{
	}

	void ServerLayer::OnAttach()
	{
		for ( auto port : mOptions.ports )
			AddPort( port );

		mContext.Run();
	}

	void ServerLayer::OnDetach()
	{
		for ( auto handle : mListeners )
			mContext.StopListener( handle );

		mContext.Stop();
	}

	void ServerLayer::OnEvent( Event& e )
	{
		e.Dispatch< NetworkOutEvent >( SL_BIND_EVENT_FUNC( SendMessage ) );
	}

	void ServerLayer::AddPort( std::uint16_t port )
	{
		sl::log::Info( "Starting listener on port {}", port );
		auto handle = mContext.Listen( port, [ this ]( sl::net::ConnectionPtr connection ) {
			sl::log::Info( "Connection received from {}", connection->GetRemoteAddress() );

			connection->OnConnect( [ this, connection ] {
				mConnections.insert( connection );
				OnConnect( connection );
			} );

			connection->OnDisconnect( [ this, connection ] {
				mConnections.erase( connection );
				OnDisconnect( connection );
			} );

			connection->OnRead( [ this ]( sl::net::Payload const& msg ) {
				OnMessage( msg );
				Application::PostEvent< NetworkInEvent >( msg );
			} );

			connection->Start();
		} );

		mListeners.push_back( handle );
	}

	bool ServerLayer::SendMessage( NetworkOutEvent& e )
	{
		auto connections_to_send = mConnections | std::views::filter( [ this, &e ]( auto connection ) { return ShouldSend( connection, e.data ); } );

		for ( auto connection : connections_to_send )
			connection->AddToQueue( e.data );

		return false;
	}
} // namespace sl::net