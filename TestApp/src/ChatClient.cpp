#include "ChatClient.h"

#include <iostream>

namespace Connection {

	class Impl : public slc::net::Connection
	{
	public:
		Impl( slc::net::Socket socket, ChatClient& client )
			: slc::net::Connection( std::move( socket ) )
			, mClient( client )
		{
		}

		void OnRead( slc::net::Payload message ) override
		{
			mClient.Receive( message );
		}
		void OnWrite( slc::net::Payload message ) override
		{
			// Do nothing for now
		}

		void OnConnect() override
		{
		}
		void OnDisconnect() override
		{
		}

	private:
		std::deque< std::string > mWriteMessages;
		ChatClient& mClient;
	};
} // namespace Connection

ChatClient::ChatClient()
{
}

void ChatClient::AddPort( std::uint16_t port )
{
	mContext.Listen( port, [ & ]( slc::net::Socket socket ) {
		mServerConnection = std::make_shared< Connection::Impl >( std::move( socket ), *this );
		mServerConnection->Start( /*is_server=*/false );
	} );
}

void ChatClient::Run()
{
	mEntryThread = std::thread( std::bind( &ChatClient::ListenForInput, this ) );
	mContext.Run();
}

void ChatClient::Receive( slc::net::Payload msg )
{
	std::string_view message( msg.As< char >(), msg.Size() );
	std::cout << message << std::endl;
}

void ChatClient::ListenForInput()
{
	try
	{
		for ( ;; )
		{
			std::string line;
			if ( !std::getline( std::cin, line ) )
				break;

			if ( mServerConnection )
			{
				slc::net::Payload msg = slc::net::Payload::Copy( line.data(), line.size() );
				mServerConnection->AddToQueue( msg );
			}
		}
	}
	catch ( std::exception& e )
	{
		std::cerr << "Input Exception: " << e.what() << "\n";
	}
}
