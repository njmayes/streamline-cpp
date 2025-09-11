#include "Context.h"

#include <asio/io_context.hpp>
#include <asio/awaitable.hpp>
#include <asio/use_awaitable.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/connect.hpp>
#include <asio/signal_set.hpp>
#include <asio/ssl.hpp>

namespace slc::net {

	struct Context::Impl
	{
		Impl( bool single_thread )
			: ssl_ctx( asio::ssl::context::tlsv13 )
		{
			ssl_ctx.set_default_verify_paths();
		}
		asio::io_context io_ctx;
		asio::ssl::context ssl_ctx;
		asio::signal_set signals{ io_ctx, SIGINT, SIGTERM };

		asio::awaitable< void > Listen( asio::ip::tcp::acceptor acceptor, std::function< void( Socket ) > on_connect )
		{
			for ( ;; )
			{
				auto asio_socket = co_await acceptor.async_accept( asio::use_awaitable );
				auto ssl_socket = asio::ssl::stream< asio::ip::tcp::socket >( std::move( asio_socket ), ssl_ctx );
				Socket socket{ std::move( ssl_socket ) };
				on_connect( std::move( socket ) );
			}
		}

		asio::awaitable< void > Connect(asio::ip::tcp::resolver resolver, std::string const& host, uint16_t port, std::function< void(Socket) > on_connect)
		{
			auto endpoints = co_await resolver.async_resolve( host, std::to_string( port ), asio::use_awaitable );
			asio::ip::tcp::socket tcp_socket{ resolver.get_executor().context() };

			co_await asio::async_connect( tcp_socket, endpoints, asio::use_awaitable );
			asio::ssl::stream< asio::ip::tcp::socket > ssl_socket{ std::move( tcp_socket ), ssl_ctx };

			Socket socket{ std::move( ssl_socket ) };
			on_connect( std::move( socket ) );
		}
	};

	Context::Context( bool single_thread )
		: mImpl{ MakeBox< Impl >( single_thread ) }
	{
	}

	Context::~Context()
	{
	}

	Context::Context( Context&& other ) noexcept
		: mImpl{ std::move( other.mImpl ) }
	{
	}

	Context& Context::operator=( Context&& other ) noexcept
	{
		mImpl = std::exchange( other.mImpl, nullptr );
		return *this;
	}

	void Context::Listen( uint16_t port, std::function< void( Socket ) > on_connect )
	{
		asio::co_spawn( mImpl->io_ctx, mImpl->Listen( asio::ip::tcp::acceptor( mImpl->io_ctx, { asio::ip::tcp::v4(), port } ), std::move( on_connect ) ), asio::detached );
	}

	void Context::Connect( const std::string& host, uint16_t port, std::function< void( Socket ) > on_connect )
	{
		asio::co_spawn( mImpl->io_ctx, mImpl->Connect( asio::ip::tcp::resolver( mImpl->io_ctx ), host, port, std::move( on_connect ) ), asio::detached );
	}

	void Context::Run()
	{
		mImpl->signals.async_wait( [ this ]( auto&&... ) { mImpl->io_ctx.stop(); } );
		mImpl->io_ctx.run();
	}

} // namespace slc::net