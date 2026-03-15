#pragma once

#include "SL/Core.h"
#include "SL/Net.h"

#include <deque>

class ServerLayer : public sl::net::ServerLayer
{
public:
	ServerLayer( sl::net::ServerContextOptions const& opts );

	void OnConnect( sl::net::ConnectionPtr participant ) override;
	void OnDisconnect( sl::net::ConnectionPtr participant ) override;
	void OnMessage( sl::net::Payload const& msg ) override;

private:
	void BroadcastMessage( std::string_view msg );

private:
	enum
	{
		max_recent_msgs = 100
	};
	std::deque< sl::net::Payload > mRecentMessages;
};

class ChatServer : public sl::Application
{
public:
	ChatServer( sl::Ref< sl::ApplicationSpecification > spec, sl::net::ServerContextOptions const& opts );
};