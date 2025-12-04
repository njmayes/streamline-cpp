#pragma once

#include "Socket.h"

namespace sl::net {

	class Connection : public std::enable_shared_from_this< Connection >
	{
	public:
		using OnMessageFunc = std::function< void( Payload const& ) >;
		using OnConnectionFunc = std::function< void() >;

		Connection( Socket socket );
		virtual ~Connection();

		Connection( Connection&& other ) noexcept;
		Connection& operator=( Connection&& other ) noexcept;

		std::string GetRemoteAddress() const;

		void OnConnect( OnConnectionFunc&& on_connect );
		void OnDisconnect( OnConnectionFunc&& on_disconnect );

		void OnRead( OnMessageFunc&& on_read );
		void OnWrite( OnMessageFunc&& on_write );

		void AddToQueue( Payload message );

		void Start();
		void Stop();

	private:
		void DoOnConnect();
		void DoOnDisconnect();

		void DoOnRead( Payload const& payload );
		void DoOnWrite( Payload const& payload );

	private:
		struct Impl;
		Box< Impl > mImpl;
	};

	using ConnectionPtr = std::shared_ptr< sl::net::Connection >;
} // namespace sl::net