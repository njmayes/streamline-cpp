#pragma once

namespace asio {
	class io_context;

	class any_io_executor; 
	
	template <typename Protocol, typename Executor>
	class basic_stream_socket;

	namespace ip {
		class tcp;

		using tcp_socket = asio::basic_stream_socket< tcp, any_io_executor >;
	}

	namespace ssl {
		template < typename Stream >
		class stream;
	}

	using ssl_stream = ssl::stream< ip::tcp_socket >;
}