#include "FileLogTarget.h"

namespace slc {
	FileLogTarget::FileLogTarget( std::string const& filename, LogLevel level )
		: ILogTarget( level )
		, mFile{ filename, std::ios::out | std::ios::app | std::ios::binary }
	{
	}

	void FileLogTarget::DoWriteTarget()
	{
		mFile.write( mBuffer.data(), mToWrite );
	}

	void FileLogTarget::DoPreFlush()
	{
		mFile.rdbuf()->pubsetbuf( mBuffer.data(), mBuffer.size() );
	}

	void FileLogTarget::DoFlush()
	{
		mFile << std::flush;
		mFile.rdbuf()->pubsetbuf( nullptr, 0 );
	}
} // namespace slc