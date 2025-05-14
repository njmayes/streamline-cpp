#include "FileLogTarget.h"

namespace slc
{
	FileLogTarget::FileLogTarget(std::string const& filename, LogLevel level)
		: ILogTarget(level)
		, mFile{ filename, std::ios::out | std::ios::app | std::ios::binary }
	{
	}

	void FileLogTarget::DoWriteTarget(std::vector<char> const& buffer, std::size_t count)
	{
		mFile.write(buffer.data(), count);
	}

	void FileLogTarget::DoPreFlush(std::vector<char>& buffer)
	{
		mFile.rdbuf()->pubsetbuf(buffer.data(), buffer.size());
	}

	void FileLogTarget::DoFlush()
	{
		mFile << std::flush;
		mFile.rdbuf()->pubsetbuf(nullptr, 0);
	}
}