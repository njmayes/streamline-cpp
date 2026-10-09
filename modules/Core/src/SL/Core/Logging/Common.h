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

	using Clock = std::chrono::steady_clock;
	using Duration = std::chrono::duration< float, std::micro >;
	using TimePoint = std::chrono::time_point< Clock, Duration >;
} // namespace sl