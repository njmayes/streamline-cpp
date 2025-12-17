#pragma once

#include "SL/Core/Common/Base.h"

#include <span>

namespace sl {

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
} // namespace sl