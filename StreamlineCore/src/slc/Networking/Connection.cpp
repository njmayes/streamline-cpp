#include "Connection.h"

#include "slc/Logging/Log.h"

#include "Context.h"

#include <set>
#include <iostream>

#include <asio/steady_timer.hpp>
#include <asio/awaitable.hpp>
#include <asio/use_awaitable.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/read.hpp>
#include <asio/write.hpp>
#include <asio/redirect_error.hpp>
#include <asio/detached.hpp>
#include <asio/signal_set.hpp>
#include <asio/co_spawn.hpp>
#include <asio/ssl.hpp>

#include <deque>

namespace slc::net {

	struct Connection::Impl
	{
		Connection* parent;

		asio::ssl::stream< asio::ip::tcp::socket > socket;
		asio::steady_timer timer;
		std::deque< Payload > write_msgs{};

		Impl( Connection* parent, Socket socket )
			: parent( parent )
			, socket( std::move( socket.GetNativeSocket() )  )
			, timer( this->socket.get_executor() )
		{
			timer.expires_at( std::chrono::steady_clock::time_point::max() );
		}

		asio::awaitable< void > Reader()
		{
			try
			{
				for ( ;; )
				{
					PayloadHeader header;
					co_await asio::async_read( socket, asio::buffer( &header, sizeof( PayloadHeader ) ), asio::use_awaitable );

					Payload read_msg( header.size );
					std::size_t n = co_await asio::async_read( socket, asio::buffer( read_msg.Data(), read_msg.Size() ), asio::use_awaitable );

					if ( n != header.size )
						throw std::runtime_error( "Read size mismatch" );

					parent->OnRead( std::move( read_msg ) );
				}
			}
			catch ( std::exception& e )
			{
				log::Error( "Reader Exception: {}", e.what() );
				parent->Stop();
			}
		}

		asio::awaitable< void > Writer()
		{
			try
			{
				while ( socket.lowest_layer().is_open() )
				{
					if ( not write_msgs.empty() )
					{
						auto& next_payload = write_msgs.front();

						PayloadHeader header;
						header.size = next_payload.Size();

						co_await asio::async_write( socket, asio::buffer( &header, sizeof( PayloadHeader ) ), asio::use_awaitable );
						co_await asio::async_write( socket, asio::buffer( next_payload.Data(), next_payload.Size() ), asio::use_awaitable );

						parent->OnWrite( std::move( next_payload ) );
						write_msgs.pop_front();
					}
					else
					{
						asio::error_code ec;
						co_await timer.async_wait( redirect_error( asio::use_awaitable, ec ) );
					}
				}
			}
			catch ( std::exception& e )
			{
				log::Error( "Writer Exception: {}", e.what() );
				parent->Stop();
			}
		}

		asio::awaitable< void > Handshake( bool is_server )
		{
			if ( is_server )
				co_await socket.async_handshake( asio::ssl::stream_base::server, asio::use_awaitable );
			else
				co_await socket.async_handshake( asio::ssl::stream_base::client, asio::use_awaitable );
		}
	};

	Connection::Connection( Socket socket )
		: mImpl{ MakeBox< Impl >( this, std::move( socket ) ) }
	{
	}

	Connection::~Connection()
	{
		Stop();
	}

	Connection::Connection( Connection&& other ) noexcept
		: mImpl{ std::move( other.mImpl ) }
	{
	}

	Connection& Connection::operator=( Connection&& other ) noexcept
	{
		mImpl = std::exchange( other.mImpl, nullptr );
		return *this;
	}

	void Connection::AddToQueue( const Payload& message )
	{
		mImpl->write_msgs.push_back( message );
		mImpl->timer.cancel_one();
	}

	void Connection::Start( bool is_server )
	{
		OnConnect();

		auto listener = [ self = shared_from_this(), is_server ]() -> asio::awaitable< void > {
			try
			{
				co_await self->mImpl->Handshake( is_server );

				asio::co_spawn( self->mImpl->socket.get_executor(), [ self ] { return self->mImpl->Reader(); }, asio::detached );
				asio::co_spawn( self->mImpl->socket.get_executor(), [ self ] { return self->mImpl->Writer(); }, asio::detached );
			}
			catch ( std::exception& e )
			{
				log::Error( "Handshake Exception: {}", e.what() );
				self->Stop();
			}
		};

		asio::co_spawn( mImpl->socket.get_executor(), listener, asio::detached );
	}

	void Connection::Stop()
	{
		OnDisconnect();

		asio::error_code ec;
		mImpl->socket.shutdown( ec );
		mImpl->socket.lowest_layer().close( ec );
		mImpl->timer.cancel();
	}
} // namespace slc::net