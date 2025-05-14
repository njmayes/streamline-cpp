#pragma once

#include "ILogTarget.h"

namespace slc {

	class ConsoleLogTarget : public ILogTarget
	{
	public:
		ConsoleLogTarget(LogLevel level);

	private:
		void DoWriteTarget(std::vector<char> const& buffer, std::size_t count) override;
		void DoPreFlush(std::vector<char>& buffer) override;
		void DoFlush() override;
	};
}