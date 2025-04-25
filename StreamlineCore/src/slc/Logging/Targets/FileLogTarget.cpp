#include "FileLogTarget.h"

namespace slc
{
	FileLogTarget::FileLogTarget(std::string const& filename, LogLevel level)
		: ILogTarget(level)
		, mFile(filename, std::ios_base::out | std::ios_base::app)
	{
	}

	void FileLogTarget::DoWriteTarget(MessageEntry const& entry)
	{
		std::string_view message{ entry.message.data(), entry.length };
		mFile << message;
	}

	void FileLogTarget::DoFlush()
	{
		mFile << std::flush;
	}
}