#include "Connection.h"

#include "SL/Core/Logging/Log.h"

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
#include <asio/strand.hpp>
#include <asio/ssl.hpp>

#include <deque>

namespace slc::net {

	struct Connection::Impl
	{
		Connection* parent{};

		InstanceType type{};

		asio::ssl::stream< asio::ip::tcp::socket > socket;
		asio::strand< asio::any_io_executor > strand{};
		asio::steady_timer timer;

		std::deque< Payload > write_msgs{};

		OnConnectionFunc on_connect{};
		OnConnectionFunc on_disconnect{};

		OnMessageFunc on_read{};
		OnMessageFunc on_write{};

		Impl( Connection* parent, Socket socket )
			: parent{ parent }
			, type( socket.GetInstanceType() )
			, socket( std::move( socket.GetNativeSocket() )  )
			, strand( this->socket.lowest_layer().get_executor() )
			, timer( strand )
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

					parent->DoOnRead( read_msg );
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
						auto next_payload = std::move( write_msgs.front() );
						write_msgs.pop_front();

						PayloadHeader header;
						header.size = next_payload.Size();

						co_await asio::async_write( socket, asio::buffer( &header, sizeof( PayloadHeader ) ), asio::use_awaitable );
						co_await asio::async_write( socket, asio::buffer( next_payload.Data(), next_payload.Size() ), asio::use_awaitable );

						parent->DoOnWrite( next_payload );
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

		asio::awaitable< void > Handshake()
		{
			switch ( type )
			{
				case InstanceType::Client:
					co_await socket.async_handshake( asio::ssl::stream_base::client, asio::use_awaitable );
					break;
				case InstanceType::Server:
					co_await socket.async_handshake( asio::ssl::stream_base::server, asio::use_awaitable );
					break;
				default:
					break;
			}
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

	std::string Connection::GetRemoteAddress() const
	{
		try
		{
			return mImpl->socket.next_layer().remote_endpoint().address().to_string();
		}
		catch ( std::exception& e )
		{
			log::Error( "Failed to get remote address: {0}", e.what() );
			return "Unknown";
		}
	}

	void Connection::OnConnect( OnConnectionFunc&& on_connect )
	{
		mImpl->on_connect = std::move( on_connect );
	}

	void Connection::OnDisconnect( OnConnectionFunc&& on_disconnect )
	{
		mImpl->on_disconnect = std::move( on_disconnect );
	}

	void Connection::OnRead( OnMessageFunc&& on_read )
	{
		mImpl->on_read = std::move( on_read );
	}

	void Connection::OnWrite( OnMessageFunc&& on_write )
	{
		mImpl->on_write = std::move( on_write );
	}

	void Connection::AddToQueue( Payload message )
	{
		asio::post( mImpl->strand, [ self = shared_from_this(), message = std::move( message ) ] {
			self->mImpl->write_msgs.push_back( std::move( message ) );
			self->mImpl->timer.cancel_one();
		} );
	}

	void Connection::Start()
	{
		DoOnConnect();

		auto listener = [ self = shared_from_this() ]() -> asio::awaitable< void > {
			try
			{
				co_await asio::dispatch( self->mImpl->strand, asio::use_awaitable );

				co_await self->mImpl->Handshake();

				asio::co_spawn( self->mImpl->strand, [ self ] { return self->mImpl->Reader(); }, asio::detached );
				asio::co_spawn( self->mImpl->strand, [ self ] { return self->mImpl->Writer(); }, asio::detached );
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
		DoOnDisconnect();

		asio::error_code ec;
		mImpl->socket.shutdown( ec );
		mImpl->socket.lowest_layer().close( ec );
		mImpl->timer.cancel();
	}

	void Connection::DoOnConnect()
	{
		if ( mImpl->on_connect )
			mImpl->on_connect();
	}

	void Connection::DoOnDisconnect()
	{
		if ( mImpl->on_disconnect )
			mImpl->on_disconnect();
	}

	void Connection::DoOnRead( Payload const& payload )
	{
		if ( mImpl->on_read )
			mImpl->on_read( payload );
	}

	void Connection::DoOnWrite( Payload const& payload )
	{
		if ( mImpl->on_write )
			mImpl->on_write( payload );
	}
} // namespace slc::net