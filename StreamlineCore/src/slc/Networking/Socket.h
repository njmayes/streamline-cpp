#pragma once

#include "slc/Common/Base.h"

#include "Forward.h"

namespace slc::net {

	// A temporary object that wraps an asio SSL socket for passing to Connection that prevents the need for asio dependency in app code.
	class Socket
	{
	public:
		Socket( asio::ssl_stream&& socket );
		~Socket();

		Socket( Socket&& other ) noexcept;
		Socket& operator=( Socket&& other ) noexcept;

		void Close();
		bool IsOpen() const;

	private:
		asio::ssl_stream&& GetNativeSocket();

		friend class Connection;

	private:
		struct Impl;
		Box< Impl > mImpl;
	};
} // namespace slc::net