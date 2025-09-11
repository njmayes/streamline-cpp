#pragma once

#include "Socket.h"
#include "Payload.h"

namespace slc::net {

	class Connection : public std::enable_shared_from_this< Connection >
	{
	public:
		Connection( Socket socket );
		virtual ~Connection();

		Connection( Connection&& other ) noexcept;
		Connection& operator=( Connection&& other ) noexcept;

		virtual void OnRead( Payload message ) = 0;
		virtual void OnWrite( Payload message ) = 0;

		virtual void OnConnect()
		{}
		virtual void OnDisconnect()
		{}

		void AddToQueue( const Payload& message );

		void Start( bool is_server );
		void Stop();

	private:
		struct Impl;
		Box< Impl > mImpl;
	};

	using ConnectionPtr = std::shared_ptr< slc::net::Connection >;
} // namespace slc::net