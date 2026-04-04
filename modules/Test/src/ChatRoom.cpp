#include "ChatRoom.h"

#include "SL/Core/Common/Time.h"

static std::string GetTimestamp()
{
	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
	std::time_t now_c = std::chrono::system_clock::to_time_t( now );

	std::tm time = sl::GetLocalTime( &now_c );

	std::string timestamp( 20, '\0' );
	std::strftime( timestamp.data(), timestamp.size(), "%F %T", &time );
	return timestamp;
}

ServerLayer::ServerLayer( sl::net::ServerContextOptions const& opts )
	: sl::net::ServerLayer( opts )
{
}

void ServerLayer::OnConnect( sl::net::ConnectionPtr participant )
{
	if ( not mRecentMessages.empty() )
	{
		// Strip null terminator from each message
		auto messages = mRecentMessages | std::views::transform( []( auto const& buffer ) { return buffer.View( 0, buffer.Size() - 1 ); } );
		auto message = sl::Buffer::Concat( messages, '\0', '\n' );
		participant->AddToQueue( message );
	}

	BroadcastMessage( std::format( "New connection from {}", participant->GetRemoteAddress() ) );
}

void ServerLayer::OnDisconnect( sl::net::ConnectionPtr participant )
{
	BroadcastMessage( std::format( "Ending connection from {}", participant->GetRemoteAddress() ) );
}

void ServerLayer::OnMessage( sl::net::Payload const& msg )
{
	auto const& new_msg = mRecentMessages.emplace_back( msg );
	while ( mRecentMessages.size() > max_recent_msgs )
		mRecentMessages.pop_front();

	sl::Application::PostEvent< sl::NetworkOutEvent >( new_msg );
}

void ServerLayer::BroadcastMessage( std::string_view msg )
{
	auto timestamp = GetTimestamp();
	auto timestamp_view = std::string_view{ timestamp.data(), timestamp.size() - 1 };

	auto text = std::format( "{}: {}", timestamp_view, msg );

	sl::net::Payload payload{};
	payload.Reserve( text.size() + 1 );
	payload.Append( text );
	payload.Push( '\0' );

	OnMessage( payload );
}

ChatServer::ChatServer( sl::Ref< sl::ApplicationSpecification > spec, sl::net::ServerContextOptions const& opts )
	: Application( spec )
{
	PushLayer< ServerLayer >( opts );
	AddLogTarget< sl::ConsoleLogTarget >( sl::LogLevel::Info );
}
