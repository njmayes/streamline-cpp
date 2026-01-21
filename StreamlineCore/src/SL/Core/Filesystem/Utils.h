#pragma once

#include <SL/Core/Types/Buffer.h>

namespace sl::fs {

	Buffer ReadToBuffer( std::filesystem::path const& filepath );
	std::string ReadToString( std::filesystem::path const& filepath );

	void Write( std::filesystem::path const& filepath, Buffer buffer );
	void Write( std::filesystem::path const& filepath, std::string_view string );

	void Create( std::filesystem::path const& filepath );
	void CreateDir( std::filesystem::path const& filepath );

	void CopyDir( std::filesystem::path const& src, std::filesystem::path const& dest );

	void Remove( std::filesystem::path const& filepath );
	void RemoveDir( std::filesystem::path const& filepath );

} // namespace sl::file