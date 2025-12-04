#pragma once

#include "SL/Core/Types/Buffer.h"
#include "SL/Core/Types/UUID.h"

namespace sl::net {

	enum class InstanceType
	{
		Client,
		Server
	};

	using Payload = Buffer;

	struct PayloadHeader
	{
		std::size_t size = 0;
	};

	using ListenerHandle = UUID;
} // namespace sl::net