#include "ChatRoom.h"

#include "slc/Logging/Log.h"

ChatRoom::ChatRoom( slc::net::ServerContextOptions const& opts )
	: mContext( opts )
{
}

void ChatRoom::AddPort( std::uint16_t port )
{
	slc::log::Info( "Starting listener on port {}", port );
	mContext.Listen( port, [ = ]( slc::net::ConnectionPtr connection ) {
		slc::log::Info( "Connection received from {}", connection->GetRemoteAddress() );

		connection->OnConnect( [ = ] { Join( connection ); } );
		connection->OnDisconnect( [ = ] { Leave( connection ); } );
		connection->OnRead( [ = ]( slc::net::Payload const& msg ) { Deliver( msg ); } );

		connection->Start();
	} );
}

void ChatRoom::Run()
{
	mContext.Run();
}

void ChatRoom::Join( slc::net::ConnectionPtr participant )
{
	mConnections.insert( participant );
	for ( auto const& msg : mRecentMessages )
		participant->AddToQueue( msg );
}

void ChatRoom::Leave( slc::net::ConnectionPtr participant )
{
	mConnections.erase( participant );
}

void ChatRoom::Deliver( slc::net::Payload msg )
{
	auto const& new_msg = mRecentMessages.emplace_back( std::move( msg ) );
	while ( mRecentMessages.size() > max_recent_msgs )
		mRecentMessages.pop_front();

	for ( auto participant : mConnections )
		participant->AddToQueue( new_msg );
}
