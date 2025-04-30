#pragma once

#include "ILogTarget.h"

namespace slc {

	class ConsoleLogTarget : public ILogTarget
	{
	public:
		ConsoleLogTarget(LogLevel level);

	private:
		void DoWriteTarget(std::vector<char> const& buffer) override;
		void DoPreFlush() override;
		void DoFlush() override;
	};
}