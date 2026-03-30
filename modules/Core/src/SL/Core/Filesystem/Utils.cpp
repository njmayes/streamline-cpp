#include "Utils.h"

#include "SL/Core/Logging/Log.h"

#include <fstream>

namespace sl::fs {

	std::size_t ReadSizeBytes( std::filesystem::path const& filepath )
	{
		std::ifstream stream( filepath, std::ios::binary | std::ios::ate );

		if ( !stream )
		{
			log::Warn( "Failed to open {}", filepath.string() );
			return 0;
		}

		const auto end = stream.tellg();

		if ( end < 0 )
		{
			log::Warn( "Failed to read size of {}", filepath.string() );
			return 0;
		}

		return static_cast< std::size_t >( end );
	}

	bool ReadToBuffer( std::filesystem::path const& filepath, char* ptr, std::size_t bytes )
	{
		std::ifstream stream( filepath, std::ios::binary );

		if ( !stream )
		{
			log::Warn( "Failed to open {}", filepath.string() );
			return false;
		}

		stream.read( ptr, static_cast< std::streamsize >( bytes ) );

		if ( !stream )
		{
			log::Warn( "Failed to read {} bytes from {}", bytes, filepath.string() );
			return false;
		}

		return true;
	}

	Buffer ReadToBuffer( std::filesystem::path const& filepath )
	{
		const auto size = ReadSizeBytes( filepath );
		Buffer out( size );

		if ( size != 0 && !ReadToBuffer( filepath, out.As< char >(), size ) )
			return {};

		return out;
	}

	std::string ReadToString( std::filesystem::path const& filepath )
	{
		const auto size = ReadSizeBytes( filepath );
		std::string out( size, '\0' );

		if ( size != 0 && !ReadToBuffer( filepath, out.data(), size ) )
			return {};

		return out;
	}

	void Write( std::filesystem::path const& filepath, Buffer buffer )
	{
		std::ofstream stream( filepath, std::ios::binary );

		if ( !stream )
		{
			// Failed to open the file
			log::Warn( "Failed to open file {}", filepath.string() );
			return;
		}

		if ( buffer.Size() == 0 )
			return;

		stream.write( buffer.As< char >(), buffer.Size() );
	}

	void Write( std::filesystem::path const& filepath, std::string_view string )
	{
		std::ofstream stream( filepath, std::ios::binary );

		if ( !stream )
		{
			// Failed to open the file
			log::Warn( "Failed to open file {}", filepath.string() );
			return;
		}

		if ( string.size() == 0 )
			return;

		stream.write( string.data(), string.size() );
	}

	void Create( std::filesystem::path const& filepath )
	{
		std::ofstream stream( filepath, std::ios::binary );
		if ( !stream )
		{
			// Failed to open the file
			log::Warn( "Failed to create file {}", filepath.string() );
			return;
		}
	}

	void CreateDir( std::filesystem::path const& filepath )
	{
		std::filesystem::create_directories( filepath );
	}

	void CopyDir( std::filesystem::path const& src, std::filesystem::path const& dest )
	{
		std::filesystem::copy( src, dest, std::filesystem::copy_options::recursive );
	}

	void Remove( std::filesystem::path const& filepath )
	{
		if ( !std::filesystem::exists( filepath ) )
		{
			log::Warn( "File does not exist!" );
			return;
		}

		std::filesystem::remove( filepath );
	}

	void RemoveDir( std::filesystem::path const& filepath )
	{
		if ( !std::filesystem::exists( filepath ) )
		{
			log::Warn( "Directory does not exist!" );
			return;
		}

		std::filesystem::remove_all( filepath );
	}
} // namespace sl::fs