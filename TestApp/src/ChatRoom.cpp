#include "ChatRoom.h"

#include "slc/Logging/Log.h"

ServerLayer::ServerLayer( slc::net::ServerContextOptions const& opts )
	: slc::net::ServerLayer( opts )
{
}

void ServerLayer::OnConnect( slc::net::ConnectionPtr participant )
{
	if ( mRecentMessages.empty() )
		return;

	// Strip null terminator from each message
	auto messages = mRecentMessages | std::views::transform( []( auto const& buffer ) { return buffer.View( 0, buffer.Size() - 1 ); } );
	auto message = slc::Buffer::Concat( messages, '\0', '\n' );

	participant->AddToQueue( message );
}

void ServerLayer::OnMessage( slc::net::Payload const& msg )
{
	auto const& new_msg = mRecentMessages.emplace_back( msg );
	while ( mRecentMessages.size() > max_recent_msgs )
		mRecentMessages.pop_front();

	slc::EventManager::Post< slc::NetworkOutEvent >( new_msg );
}

ChatServer::ChatServer( slc::Box< slc::ApplicationSpecification > spec, slc::net::ServerContextOptions const& opts )
	: Application( std::move( spec ) )
{
	PushLayer< ServerLayer >( opts );
	AddLogTarget< slc::ConsoleLogTarget >( slc::LogLevel::Info );
}
