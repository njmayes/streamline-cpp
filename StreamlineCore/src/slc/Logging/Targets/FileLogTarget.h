#pragma once

#include "ILogTarget.h"

#include <fstream>

namespace slc {

	class FileLogTarget : public ILogTarget
	{
	public:
		FileLogTarget(const std::string& filename, LogLevel level = LogLevel::Debug);

	private:
		void DoWriteTarget(std::vector<char> const& buffer, std::size_t count) override;
		void DoPreFlush(std::vector<char>& buffer) override;
		void DoFlush() override;

	private:
		std::ofstream mFile;
	};
}