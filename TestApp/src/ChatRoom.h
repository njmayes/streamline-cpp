#pragma once

#include "slc/Networking/Context.h"
#include "slc/Networking/Connection.h"

#include <set>
#include <deque>

class ChatRoom
{
public:
	ChatRoom()
	{
	}

	void AddPort( std::uint16_t port );
	void Run();

	void Join( slc::net::ConnectionPtr participant );
	void Leave( slc::net::ConnectionPtr participant );

	void Deliver( slc::net::Payload msg );

private:
	slc::net::Context mContext{ true };
	std::set< slc::net::ConnectionPtr > mConnections;
	enum
	{
		max_recent_msgs = 100
	};
	std::deque< slc::net::Payload > mRecentMessages;
};