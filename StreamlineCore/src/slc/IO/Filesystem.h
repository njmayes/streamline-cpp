#pragma once

#include <slc/Types/Buffer.h>

namespace fs = std::filesystem;

namespace slc::FileUtils {

	Buffer ReadToBuffer(const fs::path& filepath);
	std::string ReadToString(const fs::path& filepath);

	void Write(const fs::path& filepath, Buffer buffer);
	void Write(const fs::path& filepath, std::string_view string);

	void Create(const fs::path& filepath);
	void CreateDir(const fs::path& filepath);

	void CopyDir(const fs::path& src, const fs::path& dest);

	void Remove(const fs::path& filepath);
	void RemoveDir(const fs::path& filepath);

	// These return empty strings if cancelled
	fs::path OpenFile(const std::vector<std::string>& filter);
	fs::path OpenDir();
	fs::path SaveFile(const std::vector<std::string>& filter);

}