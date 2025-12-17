#include "Utils.h"

#include "SL/Core/Logging/Log.h"

#include <fstream>

namespace sl::fs {

	Buffer ReadToBuffer( const std::filesystem::path& filepath )
	{
		std::ifstream stream( filepath, std::ios::binary | std::ios::ate );

		if ( !stream )
		{
			// Failed to open the file
			log::Warn( "Failed to open {}", filepath.string() );
			return {};
		}

		std::streampos end = stream.tellg();
		stream.seekg( 0, std::ios::beg );
		size_t size = end - stream.tellg();

		if ( size == 0 )
		{
			// File is empty
			log::Warn( "File {} was empty!", filepath.string() );
			return {};
		}

		Buffer buffer( size );
		stream.read( buffer.As< char >(), size );
		stream.close();

		return buffer;
	}

	std::string ReadToString( const std::filesystem::path& filepath )
	{
		std::ifstream stream( filepath, std::ios::binary | std::ios::ate );

		if ( !stream )
		{
			// Failed to open the file
			log::Warn( "Failed to open {}", filepath.string() );
			return {};
		}

		std::streampos end = stream.tellg();
		stream.seekg( 0, std::ios::beg );
		size_t size = end - stream.tellg();

		if ( size == 0 )
		{
			// File is empty
			log::Warn( "File {} was empty!", filepath.string() );
			return {};
		}

		std::string result;
		result.resize( size );
		stream.read( &result[ 0 ], size );
		stream.close();

		return result;
	}

	void Write( const std::filesystem::path& filepath, Buffer buffer )
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

	void Write( const std::filesystem::path& filepath, std::string_view string )
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

	void Create( const std::filesystem::path& filepath )
	{
		std::ofstream stream( filepath, std::ios::binary );
		if ( !stream )
		{
			// Failed to open the file
			log::Warn( "Failed to create file {}", filepath.string() );
			return;
		}
	}

	void CreateDir( const std::filesystem::path& filepath )
	{
		std::filesystem::create_directories( filepath );
	}

	void CopyDir( const std::filesystem::path& src, const std::filesystem::path& dest )
	{
		std::filesystem::copy( src, dest, std::filesystem::copy_options::recursive );
	}

	void Remove( const std::filesystem::path& filepath )
	{
		if ( !std::filesystem::exists( filepath ) )
		{
			log::Warn( "File does not exist!" );
			return;
		}

		std::filesystem::remove( filepath );
	}

	void RemoveDir( const std::filesystem::path& filepath )
	{
		if ( !std::filesystem::exists( filepath ) )
		{
			log::Warn( "Directory does not exist!" );
			return;
		}

		std::filesystem::remove_all( filepath );
	}
} // namespace sl::file