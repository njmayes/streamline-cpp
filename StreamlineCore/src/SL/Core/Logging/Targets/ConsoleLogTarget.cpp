#include "ConsoleLogTarget.h"

#include "SL/Core/Common/Profiling.h"
#include "SL/Core/Logging/Logger.h"

#include <iostream>
#include <thread>
#include <system_error>

#ifdef SLC_PLATFORM_WINDOWS
#include "Windows.h"
#elif defined( SLC_PLATFORM_LINUX )
#include <unistd.h>
#endif

namespace {

	using namespace sl;

	static void SetupConsoleNative()
	{
#ifdef SLC_PLATFORM_WINDOWS
		HANDLE hStdOut = GetStdHandle( STD_OUTPUT_HANDLE );
		if ( hStdOut == INVALID_HANDLE_VALUE )
		{
			auto error = GetLastError();
			auto message = std::system_category().message( error );

			Logger::GetErrorLogger().Log( LogLevel::Error, "Failed to retrieve stdout handle [{}]", message );
			return;
		}

		DWORD mode = 0;
		BOOL get_success = GetConsoleMode( hStdOut, &mode );
		if ( not get_success )
		{
			auto error = GetLastError();
			auto message = std::system_category().message( error );

			Logger::GetErrorLogger().Log( LogLevel::Error, "Failed to get console mode [{}]", message );
			return;
		}

		BOOL set_success = SetConsoleMode( hStdOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING );
		if ( not set_success )
		{
			auto error = GetLastError();
			auto message = std::system_category().message( error );

			Logger::GetErrorLogger().Log( LogLevel::Error, "Failed to set console mode [{}]", message );
			return;
		}
#endif // SLC_PLATFORM_WINDOWS
	}

	static void WriteToConsoleNative( std::span< const char > buffer, std::size_t count )
	{
#ifdef SLC_PLATFORM_WINDOWS
		HANDLE hStdOut = GetStdHandle( STD_OUTPUT_HANDLE );
		if ( hStdOut == INVALID_HANDLE_VALUE )
		{
			auto error = GetLastError();
			auto message = std::system_category().message( error );

			Logger::GetErrorLogger().Log( LogLevel::Error, "Failed to retrieve stdout handle [{}]", message );
			return;
		}

		DWORD bytesWritten;
		BOOL success = WriteConsoleA( hStdOut, buffer.data(), static_cast< DWORD >( count ), &bytesWritten, nullptr );

		if ( not success )
		{
			auto error = GetLastError();
			auto message = std::system_category().message( error );

			Logger::GetErrorLogger().Log( LogLevel::Error, "Failed to write to stdout [{}]", message );
			return;
		}
#elif defined( SLC_PLATFORM_LINUX )
		auto result = ::write( STDOUT_FILENO, buffer.data(), count );
		if ( result == -1 )
		{
			auto message = std::system_category().message( errno );
			Logger::GetErrorLogger().Log( LogLevel::Error, "Failed to write to stdout [{}]", message );
			return;
		}
#else
		std::fwrite( buffer.data(), 1, buffer.mToWrite(), stdout );
#endif
	}

	static void FlushConsoleNative()
	{
#ifdef SLC_PLATFORM_WINDOWS
		// Used WriteConsoleA - flush not necessary
#elif defined( SLC_PLATFORM_LINUX )
		// Used ::write - flush not necessary
#else
		std::fflush( stdout );
#endif
	}
} // namespace

namespace sl {

	ConsoleLogTarget::ConsoleLogTarget( LogLevel level, std::map< LogLevel, ConsoleAttributes::Attribute > colours )
		: ILogTarget( level )
		, mColours( std::move( colours ) )
	{
		SetupConsoleNative();
	}

	void ConsoleLogTarget::DoWriteTarget()
	{
		WriteToConsoleNative( mBuffer, mToWrite );
	}

	void ConsoleLogTarget::DoPreFlush()
	{
	}

	void ConsoleLogTarget::DoFlush()
	{
		FlushConsoleNative();
	}

	void ConsoleLogTarget::PopulateBuffer( std::span< MessageEntry > data )
	{
		SLC_PROFILE_FUNCTION();

		auto filtered_data = data | std::views::filter( [ this ]( auto const& entry ) { return ShouldWriteMessage( entry ); } );

		mToWrite = 0;

		if ( filtered_data.empty() )
			return;

		auto first_level = filtered_data.front().level;
		WriteColourCode( first_level );

		for ( auto it = filtered_data.begin(); it != filtered_data.end(); ++it )
		{
			PopulateBufferSingleEntry( *it );

			if ( auto next = std::next( it ); next != filtered_data.end() )
				WriteColourCode( next->level );

			PopulateBufferNewLine();
		}

		WriteResetColourCode();
	}

	void ConsoleLogTarget::WriteColourCode( LogLevel level )
	{
		auto next_colour = mColours.contains( level ) ? mColours.at( level ) : ConsoleAttributes::Default;
		if ( mCurrentAttribute == next_colour )
			return;

		mCurrentAttribute = next_colour;

		WriteCharToBuffer( '\x1b' );
		WriteCharToBuffer( '[' );

		auto style = next_colour & ConsoleAttributes::StyleMask ? next_colour & ConsoleAttributes::StyleMask : ConsoleAttributes::DefaultStyle;
		auto foreground = next_colour & ConsoleAttributes::ForegroundMask ? next_colour & ConsoleAttributes::ForegroundMask : ConsoleAttributes::DefaultForeground;
		auto background = next_colour & ConsoleAttributes::BackgroundMask ? next_colour & ConsoleAttributes::BackgroundMask : ConsoleAttributes::DefaultBackground;

		WriteStyleAttribute( style >> 16 );

		WriteCharToBuffer( '3' );

		WriteColourAttribute( foreground );

		WriteCharToBuffer( ';' );
		WriteCharToBuffer( '4' );

		WriteColourAttribute( background >> 8 );

		WriteCharToBuffer( 'm' );
	}

	void ConsoleLogTarget::WriteColourAttribute( ConsoleAttributes::Attribute colour )
	{
		// Number of zero bits to right of 1 is the colour code
		// E.g. White... = 128 = 0b10000000 = 7 zeros -> '7'
		// Add '0' to get the character representation

		auto colour_value = static_cast< char >( std::countr_zero( colour ) ) + '0';
		WriteCharToBuffer( colour_value );
	}

	void ConsoleLogTarget::WriteStyleAttribute( ConsoleAttributes::Attribute style )
	{
		if ( not style )
			return;

		for ( char i = 0; i < 6; i++ )
		{
			auto bit_to_check = 1 << i;
			if ( style & bit_to_check )
			{
				auto style_value = ( i + 1 ) + '0';

				WriteCharToBuffer( style_value );
				WriteCharToBuffer( ';' );
			}
		}
	}

	void ConsoleLogTarget::WriteResetColourCode()
	{
		WriteCharToBuffer( '\x1b' );
		WriteCharToBuffer( '[' );
		WriteCharToBuffer( '0' );
		WriteCharToBuffer( 'm' );

		mCurrentAttribute = ConsoleAttributes::Default;
	}
} // namespace sl