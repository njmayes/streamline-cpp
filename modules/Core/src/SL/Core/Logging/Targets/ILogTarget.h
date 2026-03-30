#pragma once

#include "../Common.h"

#include <vector>

namespace sl {

	class ILogTarget
	{
	public:
		ILogTarget( LogLevel level )
			: mLogLevel{ level }
		{
			SetInitialBufferSize( 256_KB );
		}
		virtual ~ILogTarget() = default;

		void SetInitialBufferSize( std::size_t size );

		void WriteTarget( std::span< MessageEntry > data );

		void PreFlush()
		{
			DoPreFlush();
		}

		void Flush()
		{
			DoFlush();
		}

		void SetLogLevel( LogLevel level )
		{
			mLogLevel = level;
		}

	private:
		virtual void DoWriteTarget() = 0;
		virtual void DoPreFlush() = 0;
		virtual void DoFlush() = 0;

		virtual void PopulateBuffer( std::span< MessageEntry > data );

	protected:
		void WriteCharToBuffer( char c );
		void PopulateBufferSingleEntry( MessageEntry const& entry );
		void PopulateBufferNewLine();

		bool ShouldWriteMessage( MessageEntry const& entry ) const
		{
			return std::to_underlying( entry.level ) >= std::to_underlying( mLogLevel );
		}

	protected:
		std::vector< char > mBuffer;
		std::size_t mToWrite = 0;

	private:
		LogLevel mLogLevel;
	};
} // namespace sl