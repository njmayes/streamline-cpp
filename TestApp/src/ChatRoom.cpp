#include "ChatRoom.h"

#include "SL/Core/Logging/Log.h"

ServerLayer::ServerLayer( sl::net::ServerContextOptions const& opts )
	: sl::net::ServerLayer( opts )
{
}

void ServerLayer::OnConnect( sl::net::ConnectionPtr participant )
{
	if ( mRecentMessages.empty() )
		return;

	// Strip null terminator from each message
	auto messages = mRecentMessages | std::views::transform( []( auto const& buffer ) { return buffer.View( 0, buffer.Size() - 1 ); } );
	auto message = sl::Buffer::Concat( messages, '\0', '\n' );

	participant->AddToQueue( message );
}

void ServerLayer::OnMessage( sl::net::Payload const& msg )
{
	auto const& new_msg = mRecentMessages.emplace_back( msg );
	while ( mRecentMessages.size() > max_recent_msgs )
		mRecentMessages.pop_front();

	sl::Application::PostEvent< sl::NetworkOutEvent >( new_msg );
}

ChatServer::ChatServer( sl::Ref< sl::ApplicationSpecification > spec, sl::net::ServerContextOptions const& opts )
	: Application( spec )
{
	PushLayer< ServerLayer >( opts );
	AddLogTarget< sl::ConsoleLogTarget >( sl::LogLevel::Info );
}
