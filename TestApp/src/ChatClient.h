#pragma once

#include "slc/Networking/Context.h"
#include "slc/Networking/Connection.h"

#include <deque>
#include <thread>

class ChatClient
{
public:
	ChatClient();

	void AddPort( std::uint16_t port );
	void Run();

	void Receive( slc::net::Payload msg );

private:
	void ListenForInput();

private:
	slc::net::Context mContext{ true };
	slc::net::ConnectionPtr mServerConnection;
	enum
	{
		max_recent_msgs = 100
	};
	std::deque< slc::net::Payload > mRecentMessages;
	std::thread mEntryThread;
};