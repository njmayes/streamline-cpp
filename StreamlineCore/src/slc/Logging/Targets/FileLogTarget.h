#pragma once

#include "ILogTarget.h"

#include <fstream>

namespace slc {

	class FileLogTarget : public ILogTarget
	{
	public:
		FileLogTarget(const std::string& filename, LogLevel level = LogLevel::Debug);

	private:
		void DoWriteTarget() override;
		void DoPreFlush() override;
		void DoFlush() override;

	private:
		std::ofstream mFile;
	};
}