#include "ChatRoom.h"

namespace Connection {

	class Impl : public slc::net::Connection
	{
	public:
		Impl( slc::net::Socket socket, ChatRoom& room )
			: slc::net::Connection( std::move( socket ) )
			, mRoom( room )
		{
		}

		void OnRead( slc::net::Payload message ) override
		{
			mRoom.Deliver( message );
		}
		void OnWrite( slc::net::Payload message ) override
		{
			// Do nothing for now
		}

		void OnConnect() override
		{
			mRoom.Join( shared_from_this() );
		}
		void OnDisconnect() override
		{
			mRoom.Leave( shared_from_this() );
		}

	private:
		ChatRoom& mRoom;
	};
}

void ChatRoom::AddPort( std::uint16_t port )
{
	mContext.Listen( port, [ & ]( slc::net::Socket socket ) {
		auto session = std::make_shared< Connection::Impl >( std::move( socket ), *this );
		session->Start( /*is_server=*/true );
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
