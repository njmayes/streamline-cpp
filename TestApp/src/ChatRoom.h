#pragma once

#include "streamline.h"

#include <set>
#include <deque>

class ServerLayer : public slc::net::ServerLayer
{
public:
	ServerLayer( slc::net::ServerContextOptions const& opts );

	void OnConnect( slc::net::ConnectionPtr participant ) override;
	void OnMessage( slc::net::Payload const& msg ) override;

private:
	enum
	{
		max_recent_msgs = 100
	};
	std::deque< slc::net::Payload > mRecentMessages;
};

class ChatServer : public slc::Application
{
public:
	ChatServer( slc::Box< slc::ApplicationSpecification > spec, slc::net::ServerContextOptions const& opts );
};