#pragma once

#include "slc/Types/Buffer.h"
#include "slc/Types/UUID.h"

namespace slc::net {

	using Payload = Buffer;

	struct PayloadHeader
	{
		std::size_t size = 0;
	};
} // namespace slc::net