#pragma once

#include "slc/Common/Base.h"

#include "Socket.h"
#include "Forward.h"

namespace slc::net {

	class Context
	{
	public:
		Context( bool single_thread = false );
		~Context();

		Context( Context&& other ) noexcept;
		Context& operator=( Context&& other ) noexcept;

		void Listen( uint16_t port, std::function< void( Socket ) > on_connect );
		void Connect( const std::string& host, uint16_t port, std::function< void( Socket ) > on_connect );

		void Run();	

	private:
		struct Impl;
		Box< Impl > mImpl;
	};
} // namespace slc::net