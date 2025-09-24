#pragma once

#include <slc/Types/Buffer.h>

namespace slc::fs {

	Buffer ReadToBuffer( const std::filesystem::path& filepath );
	std::string ReadToString( const std::filesystem::path& filepath );

	void Write( const std::filesystem::path& filepath, Buffer buffer );
	void Write( const std::filesystem::path& filepath, std::string_view string );

	void Create( const std::filesystem::path& filepath );
	void CreateDir( const std::filesystem::path& filepath );

	void CopyDir( const std::filesystem::path& src, const std::filesystem::path& dest );

	void Remove( const std::filesystem::path& filepath );
	void RemoveDir( const std::filesystem::path& filepath );

} // namespace slc::file