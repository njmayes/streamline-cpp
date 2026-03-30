#include "ILogTarget.h"

#include "SL/Core/Common/Base.h"
#include "SL/Core/Common/Profiling.h"

#include <ranges>

namespace sl {

	void ILogTarget::SetInitialBufferSize( std::size_t size )
	{
		mBuffer.resize( size );
	}

	void ILogTarget::WriteTarget( std::span< MessageEntry > data )
	{
		SL_PROFILE_FUNCTION();

		PopulateBuffer( data );

		if ( mToWrite == 0 )
			return;

		DoWriteTarget();
	}

	void ILogTarget::PopulateBuffer( std::span< MessageEntry > data )
	{
		SL_PROFILE_FUNCTION();

		mToWrite = 0;

		for ( auto const& entry : data )
		{
			if ( ShouldWriteMessage( entry ) )
			{
				PopulateBufferSingleEntry( entry );
				PopulateBufferNewLine();
			}
		}
	}

	void ILogTarget::PopulateBufferSingleEntry( MessageEntry const& entry )
	{
		while ( mToWrite + entry.length >= mBuffer.size() ) [[unlikely]]
			mBuffer.resize( mBuffer.size() * 2 );

		std::memcpy( mBuffer.data() + mToWrite, entry.message.data(), entry.length );
		mToWrite += entry.length;
	}

	void ILogTarget::PopulateBufferNewLine()
	{
#ifdef SL_PLATFORM_WINDOWS
		WriteCharToBuffer( '\r' );
#endif // SL_PLATFORM_WINDOWS

		WriteCharToBuffer( '\n' );
	}

	void ILogTarget::WriteCharToBuffer( char c )
	{
		if ( mToWrite >= mBuffer.size() ) [[unlikely]]
			mBuffer.resize( mBuffer.size() * 2 );

		mBuffer[ mToWrite ] = c;
		mToWrite++;
	}
} // namespace sl