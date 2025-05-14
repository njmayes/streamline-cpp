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

	static void WriteToConsoleNative(std::vector<char> const& buffer, std::size_t count)
	{
#ifdef SLC_PLATFORM_WINDOWS
		HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
		ASSERT(hStdOut != INVALID_HANDLE_VALUE, "failed to retrieve stdout handle");

		std::wstring wideMessage(buffer.begin(), buffer.begin() + count);

		DWORD bytesWritten;
		BOOL success = WriteConsole(hStdOut, wideMessage.data(), static_cast<DWORD>(wideMessage.size()), &bytesWritten, nullptr);

		ASSERT(success, "Failed to write to stdout");
#elif defined(SLC_PLATFORM_LINUX)
		auto result = ::write(STDOUT_FILENO, mMessageBuffer.data(), count);
		ASSERT(result != -1, "Failed to write to stdout")
#else
		std::fwrite(buffer.data(), 1, buffer.size(), stdout);
#endif
	}

	static void FlushConsoleNative()
	{
#ifdef SLC_PLATFORM_WINDOWS
		HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
		ASSERT(hStdOut != INVALID_HANDLE_VALUE, "failed to retrieve stdout handle");

		FlushFileBuffers(hStdOut);
#elif defined(SLC_PLATFORM_LINUX)
		// Used ::write - flush not necessary
#else
		std::fflush(stdout);
#endif
	}
}

namespace slc {

	ConsoleLogTarget::ConsoleLogTarget(LogLevel level)
		: ILogTarget(level)
	{
	}

	void ConsoleLogTarget::DoWriteTarget(std::vector<char> const& buffer, std::size_t count)
	{
		WriteToConsoleNative(buffer, count);
	}

	void ConsoleLogTarget::DoPreFlush(std::vector<char>& buffer)
	{
	}

	void ConsoleLogTarget::DoFlush()
	{
		FlushConsoleNative();
	}
}