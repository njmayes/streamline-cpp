#include "Socket.h"

#include <asio/ip/tcp.hpp>
#include <asio/awaitable.hpp>
#include <asio/write.hpp>
#include <asio/detached.hpp>
#include <asio/co_spawn.hpp>
#include <asio/use_awaitable.hpp>
#include <asio/ssl.hpp>

namespace slc::net {

	struct Socket::Impl
	{
		Impl( asio::ssl::stream< asio::ip::tcp::socket > socket )
			: socket( std::move( socket ) )
		{
		}
		asio::ssl::stream< asio::ip::tcp::socket > socket;
	};

	Socket::Socket( asio::ssl_stream&& socket )
		: mImpl{ MakeBox< Impl >( std::move( socket ) ) }
	{
	}

	Socket::~Socket()
	{}

	Socket::Socket( Socket&& other ) noexcept
		: mImpl{ std::move( other.mImpl ) }
	{
	}

	Socket& Socket::operator=( Socket&& other ) noexcept
	{
		mImpl = std::exchange( other.mImpl, nullptr );
		return *this;
	}

	bool Socket::IsOpen() const
	{
		return mImpl->socket.lowest_layer().is_open();
	}

	asio::ssl_stream& Socket::GetNativeSocket()
	{
		return mImpl->socket;
	}
} // namespace slc::net