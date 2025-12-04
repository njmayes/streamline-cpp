#include "Socket.h"

#include <SL/Core/Logging/Log.h>

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
		Impl( asio::ssl::stream< asio::ip::tcp::socket > socket, InstanceType type )
			: socket( std::move( socket ) )
			, type( type )
		{
		}
		asio::ssl::stream< asio::ip::tcp::socket > socket;
		InstanceType type;
	};

	Socket::Socket( asio::ssl_stream&& socket, InstanceType type )
		: mImpl{ MakeBox< Impl >( std::move( socket ), type ) }
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

	asio::ssl_stream&& Socket::GetNativeSocket()
	{
		return std::move( mImpl->socket );
	}

	InstanceType Socket::GetInstanceType() const
	{
		return mImpl->type;
	}
} // namespace slc::net