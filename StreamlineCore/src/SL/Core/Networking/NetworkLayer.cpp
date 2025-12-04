#include "NetworkLayer.h"

#include "SL/Core/Common/Application.h"
#include "SL/Core/Logging/Log.h"

namespace slc::net {

	/*
		Client
	*/

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
		mContext.Connect( host, port, [ = ]( slc::net::ConnectionPtr connection ) {
			slc::log::Info( "Client connected to {}:{}", host, port );

			mServerConnection = connection;

			mServerConnection->OnConnect( [ this, connection ] {
				OnConnect( connection );
			} );
			mServerConnection->OnDisconnect( [ this, connection ] { OnDisconnect( connection ); } );

			mServerConnection->OnRead( [ this ]( slc::net::Payload const& msg ) {
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


	/*
		Server
	*/

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
		e.Dispatch< NetworkOutEvent >( SLC_BIND_EVENT_FUNC( SendMessage ) );
	}

	void ServerLayer::AddPort( std::uint16_t port )
	{
		slc::log::Info( "Starting listener on port {}", port );
		auto handle = mContext.Listen( port, [ this ]( slc::net::ConnectionPtr connection ) {
			slc::log::Info( "Connection received from {}", connection->GetRemoteAddress() );

			connection->OnConnect( [ this, connection ] {
				mConnections.insert( connection );
				OnConnect( connection );
			} );

			connection->OnDisconnect( [ this, connection ] {
				mConnections.erase( connection );
				OnDisconnect( connection );
			} );

			connection->OnRead( [ this ]( slc::net::Payload const& msg ) {
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
} // namespace slc::net