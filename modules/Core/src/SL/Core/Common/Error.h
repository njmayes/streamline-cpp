#pragma once

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <source_location>

namespace sl::detail {
	[[noreturn]] inline void ReportFatalError(
		char const* kind,
		char const* expression,
		char const* message,
		std::source_location location = std::source_location::current()
	)
	{
		std::fprintf(
			stderr,
			"\n[%s] %s\nFile: %s\nLine: %u\nFunction: %s\n",
			kind ? kind : "Fatal",
			message ? message : "",
			location.file_name(),
			location.line(),
			location.function_name()
		);

		if ( expression && *expression )
		{
			std::fprintf( stderr, "Expression: %s\n", expression );
		}

		std::fflush( stderr );
		std::abort();
	}
} // namespace sl::detail