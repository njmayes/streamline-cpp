#include "ConsoleLogTarget.h"

#include <iostream>

namespace slc {

	ConsoleLogTarget::ConsoleLogTarget(LogLevel level)
		: ILogTarget(level)
	{
	}

	void ConsoleLogTarget::DoWriteTarget(MessageEntry const& entry)
	{
		std::string_view message{ entry.message.data(), entry.length };
		std::cout << message;
	}

	void ConsoleLogTarget::DoFlush()
	{
		std::cout << std::flush;
	}
}