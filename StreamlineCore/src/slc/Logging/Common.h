#pragma once

#include "slc/Common/Base.h"

#include <span>

namespace slc {

	enum class LogLevel
	{
		Trace,
		Debug,
		Info,
		Warning,
		Error,
		Fatal
	};

	using MessageBuffer = std::span< char >;

	struct MessageEntry
	{
		MessageBuffer message;
		std::size_t length;
		LogLevel level;
	};
} // namespace slc