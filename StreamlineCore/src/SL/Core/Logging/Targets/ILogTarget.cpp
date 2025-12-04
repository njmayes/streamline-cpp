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
		SLC_PROFILE_FUNCTION();

		PopulateBuffer( data );

		if ( mToWrite == 0 )
			return;

		DoWriteTarget();
	}

	void ILogTarget::PopulateBuffer( std::span< MessageEntry > data )
	{
		SLC_PROFILE_FUNCTION();

		auto filtered_data = data | std::views::filter( [ this ]( auto const& entry ) { return ShouldWriteMessage( entry ); } ) | std::ranges::to< std::vector >();

		mToWrite = 0;

		for ( auto& entry : filtered_data )
		{
			PopulateBufferSingleEntry( entry );
			PopulateBufferNewLine();
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
#ifdef SLC_PLATFORM_WINDOWS
		WriteCharToBuffer( '\r' );
#endif // SLC_PLATFORM_WINDOWS

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