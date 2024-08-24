#include "Filesystem.h"

#include <fstream>

#include <portable-file-dialogs.h>

namespace slc {

	fs::path FileUtils::LabRoot()
	{
		int depthCount = 0;
		fs::path rootDirectory = fs::current_path();

		while (rootDirectory.stem() != "LabyrinthEngine")
		{
			if (depthCount > 8)
			{
				LOG("Could not locate labyrinth root directory!");
				return fs::path();
			}

			rootDirectory = rootDirectory.parent_path();
			depthCount++;
		}

		return rootDirectory;
	}

	Buffer FileUtils::Read(const fs::path& filepath)
	{
		std::ifstream stream(filepath, std::ios::binary | std::ios::ate);

		if (!stream)
		{
			// Failed to open the file
			LOG("Failed to open {}", filepath);
			return nullptr;
		}

		std::streampos end = stream.tellg();
		stream.seekg(0, std::ios::beg);
		size_t size = end - stream.tellg();

		if (size == 0)
		{
			// File is empty
			LOG("File {} was empty!", filepath);
			return nullptr;
		}

		Buffer buffer(size);
		stream.read(buffer.As<char>(), size);
		stream.close();

		return buffer;
	}

	void FileUtils::Read(const fs::path& filepath, std::string& string)
	{
		std::ifstream stream(filepath, std::ios::binary | std::ios::ate);

		if (!stream)
		{
			// Failed to open the file
			LOG("Failed to open {}", filepath);
			return;
		}

		std::streampos end = stream.tellg();
		stream.seekg(0, std::ios::beg);
		size_t size = end - stream.tellg();

		if (size == 0)
		{
			// File is empty
			LOG("File {} was empty!", filepath);
			return;
		}

		string.resize(size);
		stream.read(&string[0], size);
		stream.close();
	}

	void FileUtils::Write(const fs::path& filepath, Buffer buffer)
	{
		std::ofstream stream(filepath, std::ios::binary);

		if (!stream)
		{
			// Failed to open the file
			LOG("Failed to open file {}", filepath);
			return;
		}

		if (buffer.Size() == 0)
			return;

		stream.write(buffer.As<char>(), buffer.Size());
	}

	void FileUtils::Write(const fs::path& filepath, std::string_view string)
	{
		std::ofstream stream(filepath, std::ios::binary);

		if (!stream)
		{
			// Failed to open the file
			LOG("Failed to open file {}", filepath);
			return;
		}

		if (string.size() == 0)
			return;

		stream.write(string.data(), string.size());
	}

	void FileUtils::Create(const fs::path& filepath)
	{
		std::ofstream stream(filepath, std::ios::binary);
		if (!stream)
		{
			// Failed to open the file
			LOG("Failed to create file {}", filepath);
			return;
		}
	}

	void FileUtils::CreateDir(const fs::path& filepath)
	{
		fs::create_directories(filepath);
	}

	void FileUtils::CopyDir(const fs::path& src, const fs::path& dest)
	{
		fs::copy(src, dest, fs::copy_options::recursive);
	}

	void FileUtils::Remove(const fs::path& filepath)
	{
		if (!fs::exists(filepath))
		{
			LOG("File does not exist!");
			return;
		}

		fs::remove(filepath);
	}

	void FileUtils::RemoveDir(const fs::path& filepath)
	{
		if (!fs::exists(filepath))
		{
			LOG("Directory does not exist!");
			return;
		}

		fs::remove_all(filepath);
	}

	fs::path FileUtils::OpenFile(const std::vector<std::string>& filter)
	{
		auto selection = pfd::open_file("Select a file", ".", filter).result();
		if (!selection.empty())
			return selection[0];

		return fs::path();
	}

	fs::path FileUtils::OpenDir()
	{
		return pfd::select_folder("Select a folder", ".").result();
	}

	fs::path FileUtils::SaveFile(const std::vector<std::string>& filter)
	{
		return pfd::save_file("Save file as", ".", filter).result();
	}
}