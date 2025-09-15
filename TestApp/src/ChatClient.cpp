#include "ChatClient.h"

#include "slc/Logging/Log.h"
#include "slc/Common/Time.h"

#include <iostream>

static std::string GetTimestamp()
{
	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
	std::time_t now_c = std::chrono::system_clock::to_time_t( now );

	std::tm time = slc::GetLocalTime( &now_c );

	std::string timestamp( 20, '\0' );
	std::strftime( timestamp.data(), timestamp.size(), "%F %T", &time );
	return timestamp;
}

ChatClient::ChatClient( slc::net::ClientContextOptions const& opts )
	: mContext( opts )
{
}

void ChatClient::Connect( std::string const& host, std::uint16_t port )
{
	mContext.Connect( host, port, [ = ]( slc::net::ConnectionPtr connection ) {
		slc::log::Info( "Client connected to {}:{}", host, port );

		mServerConnection = connection;

		mServerConnection->OnConnect( [ this ] {
			mEntryThread = std::thread( std::bind( &ChatClient::ListenForInput, this ) );
			slc::log::Info( "Please enter your name..." );
		} );

		mServerConnection->OnRead( [ this ]( slc::net::Payload const& msg ) { Receive( msg ); } );
		mServerConnection->Start();
	} );
}

void ChatClient::Run()
{
	mContext.Run();
}

void ChatClient::Receive( slc::net::Payload msg )
{
	std::string_view message( msg.As< char >(), msg.Size() );
	std::cout << message << '\n';
}

// static std::string GenRandomString( const int len )
//{
//	static const char alphanum[] =
//		"0123456789"
//		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
//		"abcdefghijklmnopqrstuvwxyz";
//	std::string tmp_s;
//	tmp_s.reserve( len );
//
//	for ( int i = 0; i < len; ++i )
//	{
//		tmp_s += alphanum[ rand() % ( sizeof( alphanum ) - 1 ) ];
//	}
//
//	return tmp_s;
// }

void ChatClient::ListenForInput()
{
	try
	{
		for ( ;; )
		{
			std::string line; // = GenRandomString( 128 );
			if ( !std::getline( std::cin, line ) )
				break;

			if (mUsername.empty())
			{
				mUsername = std::move( line );
				slc::log::Info( "Welcome {}!", mUsername );
			}
			else if ( mServerConnection and not line.empty() )
			{
				auto timestamp = GetTimestamp();
				auto text = std::format( "{}: [{}] {}", timestamp, mUsername, line );

				slc::net::Payload msg{};
				msg.Reserve( text.size() + 1 );
				msg.Append( text );
				msg.Push( '\0' );
				mServerConnection->AddToQueue( std::move( msg ) );
			}
		}
	}
	catch ( std::exception& e )
	{
		slc::log::Error( "Input Exception: {}", e.what() );
	}
}
