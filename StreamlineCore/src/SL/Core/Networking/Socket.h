#pragma once

#include "SL/Core/Common/Base.h"

#include "Forward.h"
#include "Common.h"

namespace slc::net {

	class Connection;

	// A temporary object that wraps an asio SSL socket for passing to Connection. Should not be used by app code.
	class Socket
	{
	public:
		Socket( asio::ssl_stream&& socket, InstanceType type );
		~Socket();

		Socket( Socket&& other ) noexcept;
		Socket& operator=( Socket&& other ) noexcept;

	private:
		asio::ssl_stream&& GetNativeSocket();
		InstanceType GetInstanceType() const;

		friend class Connection;

	private:
		struct Impl;
		Box< Impl > mImpl;
	};
} // namespace slc::net