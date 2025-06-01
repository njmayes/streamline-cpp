#include "Filesystem.h"

#include "slc/Logging/Log.h"

#include <fstream>

#include <portable-file-dialogs.h>

namespace slc::FileUtils {

	Buffer ReadToBuffer( const fs::path& filepath )
	{
		std::ifstream stream( filepath, std::ios::binary | std::ios::ate );

		if ( !stream )
		{
			// Failed to open the file
			Log::Warn( "Failed to open {}", filepath.string() );
			return nullptr;
		}

		std::streampos end = stream.tellg();
		stream.seekg( 0, std::ios::beg );
		size_t size = end - stream.tellg();

		if ( size == 0 )
		{
			// File is empty
			Log::Warn( "File {} was empty!", filepath.string() );
			return nullptr;
		}

		Buffer buffer( size );
		stream.read( buffer.As< char >(), size );
		stream.close();

		return buffer;
	}

	std::string ReadToString( const fs::path& filepath )
	{
		std::ifstream stream( filepath, std::ios::binary | std::ios::ate );

		if ( !stream )
		{
			// Failed to open the file
			Log::Warn( "Failed to open {}", filepath.string() );
			return {};
		}

		std::streampos end = stream.tellg();
		stream.seekg( 0, std::ios::beg );
		size_t size = end - stream.tellg();

		if ( size == 0 )
		{
			// File is empty
			Log::Warn( "File {} was empty!", filepath.string() );
			return {};
		}

		std::string result;
		result.resize( size );
		stream.read( &result[ 0 ], size );
		stream.close();

		return result;
	}

	void Write( const fs::path& filepath, Buffer buffer )
	{
		std::ofstream stream( filepath, std::ios::binary );

		if ( !stream )
		{
			// Failed to open the file
			Log::Warn( "Failed to open file {}", filepath.string() );
			return;
		}

		if ( buffer.Size() == 0 )
			return;

		stream.write( buffer.As< char >(), buffer.Size() );
	}

	void Write( const fs::path& filepath, std::string_view string )
	{
		std::ofstream stream( filepath, std::ios::binary );

		if ( !stream )
		{
			// Failed to open the file
			Log::Warn( "Failed to open file {}", filepath.string() );
			return;
		}

		if ( string.size() == 0 )
			return;

		stream.write( string.data(), string.size() );
	}

	void Create( const fs::path& filepath )
	{
		std::ofstream stream( filepath, std::ios::binary );
		if ( !stream )
		{
			// Failed to open the file
			Log::Warn( "Failed to create file {}", filepath.string() );
			return;
		}
	}

	void CreateDir( const fs::path& filepath )
	{
		fs::create_directories( filepath );
	}

	void CopyDir( const fs::path& src, const fs::path& dest )
	{
		fs::copy( src, dest, fs::copy_options::recursive );
	}

	void Remove( const fs::path& filepath )
	{
		if ( !fs::exists( filepath ) )
		{
			Log::Warn( "File does not exist!" );
			return;
		}

		fs::remove( filepath );
	}

	void RemoveDir( const fs::path& filepath )
	{
		if ( !fs::exists( filepath ) )
		{
			Log::Warn( "Directory does not exist!" );
			return;
		}

		fs::remove_all( filepath );
	}

	fs::path OpenFile( const std::vector< std::string >& filter )
	{
		auto selection = pfd::open_file( "Select a file", ".", filter ).result();
		if ( !selection.empty() )
			return selection[ 0 ];

		return fs::path();
	}

	fs::path OpenDir()
	{
		return pfd::select_folder( "Select a folder", "." ).result();
	}

	fs::path SaveFile( const std::vector< std::string >& filter )
	{
		return pfd::save_file( "Save file as", ".", filter ).result();
	}
} // namespace slc::FileUtils