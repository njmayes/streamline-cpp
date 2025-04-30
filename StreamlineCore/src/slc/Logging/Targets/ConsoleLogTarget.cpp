#include "ConsoleLogTarget.h"

#include <iostream>
#include <thread>

#include "slc/Common/Base.h"

#ifdef SLC_PLATFORM_WINDOWS
#include "Windows.h"
#elif defined(SLC_PLATFORM_WINDOWS)
#include <unistd.h>
#endif

namespace {

	static void WriteToConsoleFast(std::vector<char> const& buffer)
	{
#ifdef SLC_PLATFORM_WINDOWS
		HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
		ASSERT(hStdOut != INVALID_HANDLE_VALUE, "failed to retrieve stdout handle");

		std::wstring wideMessage(buffer.begin(), buffer.end());

		DWORD bytesWritten;
		BOOL success = WriteConsole(hStdOut, wideMessage.data(), static_cast<DWORD>(wideMessage.size()), &bytesWritten, nullptr);

		ASSERT(success, "Failed to write to stdout");
#elif defined(SLC_PLATFORM_LINUX)
		auto result = ::write(STDOUT_FILENO, mMessageBuffer.data(), mMessageBuffer.size());
		ASSERT(result != -1, "Failed to write to stdout")
#endif
	}
}

namespace slc {

	ConsoleLogTarget::ConsoleLogTarget(LogLevel level)
		: ILogTarget(level)
	{
	}

	void ConsoleLogTarget::DoWriteTarget(std::vector<char> const& buffer)
	{
		WriteToConsoleFast(buffer);
		//std::fwrite(buffer.data(), 1, buffer.size(), stdout);
	}

	void ConsoleLogTarget::DoPreFlush()
	{
	}

	void ConsoleLogTarget::DoFlush()
	{
		std::fflush(stdout);
	}
}