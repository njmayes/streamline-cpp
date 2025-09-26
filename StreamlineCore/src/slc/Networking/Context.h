#pragma once

#include "slc/Common/Base.h"

#include "Connection.h"

namespace slc::net {

	struct ContextOptions
	{
		std::size_t num_threads = 1;
	};

	struct ClientContextOptions final : public ContextOptions
	{
		std::string host;
		std::uint16_t port;

		std::optional< std::string > cert_file{};
	};

	struct ServerContextOptions final : public ContextOptions
	{
		std::vector< std::uint16_t > ports;

		std::string cert_file{};
		std::string key_file{};
	};

	template < typename options_t >
	concept ContextOptionsType = std::derived_from< options_t, ContextOptions > and ( not std::same_as< options_t, ContextOptions > );

	class Context
	{
	public:
		Context();

		template < ContextOptionsType options_t >
		Context( options_t const& options )
			: Context( std::is_same_v< options_t, ClientContextOptions > ? InstanceType::Client : InstanceType::Server, options )
		{
		}

		~Context();

		Context( Context&& other ) noexcept;
		Context& operator=( Context&& other ) noexcept;

		Context( Context const& ) = delete;
		Context& operator=( Context const& ) = delete;

		ListenerHandle Listen( uint16_t port, std::function< void( ConnectionPtr ) > on_connect );
		void StopListener( ListenerHandle handle );

		void Connect( const std::string& host, uint16_t port, std::function< void( ConnectionPtr ) > on_connect );

		void Run();
		void Stop();

	private:
		Context( InstanceType type, ContextOptions const& options );

	private:
		struct Impl;
		Box< Impl > mImpl;
	};
} // namespace slc::net