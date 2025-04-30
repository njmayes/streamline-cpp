#include "FileLogTarget.h"

namespace slc
{
	FileLogTarget::FileLogTarget(std::string const& filename, LogLevel level)
		: ILogTarget(level)
		, mFile(filename, std::ios_base::out | std::ios_base::app)
	{
	}

	void FileLogTarget::DoWriteTarget(std::vector<char> const& buffer)
	{
		mFile << buffer.data();
	}

	void FileLogTarget::DoFlush()
	{
		mFile << std::flush;
	}
}