#include "Context.h"

#include <asio/io_context.hpp>
#include <asio/awaitable.hpp>
#include <asio/use_awaitable.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/connect.hpp>
#include <asio/signal_set.hpp>
#include <asio/this_coro.hpp>
#include <asio/cancellation_signal.hpp>
#include <asio/ssl.hpp>

#include <thread>

namespace slc::net {

	struct Context::Impl
	{
		Impl( InstanceType type, ContextOptions const& options )
			: type{ type }
			, io_ctx{ options.num_threads > 1 ? ASIO_CONCURRENCY_HINT_SAFE : ASIO_CONCURRENCY_HINT_1 }
			, ssl_ctx( asio::ssl::context::tlsv13 )
			, num_threads( options.num_threads )
		{
			switch ( type )
			{
				case InstanceType::Client:
				{
					auto const& client_options = static_cast< ClientContextOptions const& >( options );
					if ( client_options.cert_file )
						ssl_ctx.load_verify_file( *client_options.cert_file );
					else
						ssl_ctx.set_default_verify_paths();
					ssl_ctx.set_verify_mode( asio::ssl::verify_none );
					break;
				}
				case InstanceType::Server:
				{
					auto const& server_options = static_cast< ServerContextOptions const& >( options );
					ssl_ctx.use_certificate_file( server_options.cert_file, asio::ssl::context::pem );
					ssl_ctx.use_private_key_file( server_options.key_file, asio::ssl::context::pem );
					break;
				}
			}
		}

		InstanceType type;

		std::size_t num_threads{};

		asio::io_context io_ctx;
		asio::ssl::context ssl_ctx;
		asio::signal_set signals{ io_ctx, SIGINT, SIGTERM };

		std::map< ListenerHandle, std::function< void() > > listeners{};

		asio::awaitable< void > Listen( asio::ip::tcp::acceptor acceptor, ListenerHandle handle, std::function< void( ConnectionPtr ) > on_connect )
		{
			listeners[ handle ] = [ & ] { acceptor.cancel(); };

			try
			{
				for ( ;; )
				{
					auto asio_socket = co_await acceptor.async_accept( asio::use_awaitable );
					auto ssl_socket = asio::ssl::stream< asio::ip::tcp::socket >( std::move( asio_socket ), ssl_ctx );

					Socket socket{ std::move( ssl_socket ), type };
					on_connect( std::make_shared< Connection >( std::move( socket ) ) );
				}
			}
			catch ( const asio::system_error& e )
			{
				if ( e.code() == asio::error::operation_aborted )
				{
					co_return; // acceptor was cancelled
				}
				throw;
			}
		}

		asio::awaitable< void > Connect( asio::ip::tcp::resolver resolver, std::string const& host, uint16_t port, std::function< void( ConnectionPtr ) > on_connect )
		{
			asio::io_context& ctx = static_cast< asio::io_context& >( resolver.get_executor().context() );
			auto endpoints = co_await resolver.async_resolve( host, std::to_string( port ), asio::use_awaitable );

			asio::ip::tcp::socket tcp_socket{ ctx };
			co_await asio::async_connect( tcp_socket, endpoints, asio::use_awaitable );

			asio::ssl::stream< asio::ip::tcp::socket > ssl_socket{ std::move( tcp_socket ), ssl_ctx };

			Socket socket{ std::move( ssl_socket ), type };
			on_connect( std::make_shared< Connection >( std::move( socket ) ) );
		}
	};

	Context::Context( InstanceType type, ContextOptions const& options )
		: mImpl{ MakeBox< Impl >( type, options ) }
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

	ListenerHandle Context::Listen( uint16_t port, std::function< void( ConnectionPtr ) > on_connect )
	{
		ListenerHandle handle{};
		asio::co_spawn( mImpl->io_ctx, mImpl->Listen( asio::ip::tcp::acceptor( mImpl->io_ctx, { asio::ip::tcp::v4(), port } ), handle, std::move( on_connect ) ), asio::detached );
		return handle;
	}

	void Context::StopListener( ListenerHandle handle )
	{
		if ( not mImpl->listeners.contains( handle ) )
			return;

		std::invoke( mImpl->listeners.at( handle ) );
	}

	void Context::Connect( const std::string& host, uint16_t port, std::function< void( ConnectionPtr ) > on_connect )
	{
		asio::co_spawn( mImpl->io_ctx, mImpl->Connect( asio::ip::tcp::resolver( mImpl->io_ctx ), host, port, std::move( on_connect ) ), asio::detached );
	}

	void Context::Run()
	{
		mImpl->signals.async_wait( [ this ]( auto&&... ) { mImpl->io_ctx.stop(); } );

		auto threads = std::vector< std::jthread >( mImpl->num_threads );
		for (auto& thread : threads)
		{
			thread = std::jthread( [ this ] {
				mImpl->io_ctx.run();
			} );
		}
	}

} // namespace slc::net