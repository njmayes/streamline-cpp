#pragma once

#include <SL/Core/Types/Buffer.h>

namespace sl::fs {

	std::size_t ReadSizeBytes( std::filesystem::path const& filepath );
	bool ReadToBuffer( std::filesystem::path const& filepath, char* ptr, std::size_t bytes );

	Buffer ReadToBuffer( std::filesystem::path const& filepath );
	std::string ReadToString( std::filesystem::path const& filepath );

	template < typename char_t >
		requires( sizeof( char_t ) == 1 )
	std::vector< char_t > ReadToVector( std::filesystem::path const& filepath )
	{
		const auto size = ReadSizeBytes( filepath );
		std::vector< char_t > out( size );

		if ( size != 0 && !ReadToBuffer( filepath, reinterpret_cast< char* >( out.data() ), size ) )
			return {};

		return out;
	}

	void Write( std::filesystem::path const& filepath, Buffer buffer );
	void Write( std::filesystem::path const& filepath, std::string_view string );

	void Create( std::filesystem::path const& filepath );
	void CreateDir( std::filesystem::path const& filepath );

	void CopyDir( std::filesystem::path const& src, std::filesystem::path const& dest );

	void Remove( std::filesystem::path const& filepath );
	void RemoveDir( std::filesystem::path const& filepath );

} // namespace sl::fs