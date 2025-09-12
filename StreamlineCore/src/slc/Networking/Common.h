#pragma once

#include "slc/Types/Buffer.h"
#include "slc/Types/UUID.h"

namespace slc::net {

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
} // namespace slc::net