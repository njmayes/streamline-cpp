#pragma once

#include "ILogTarget.h"

namespace slc {

	class ConsoleLogTarget : public ILogTarget
	{
	public:
		ConsoleLogTarget(LogLevel level);

	private:
		void DoWriteTarget(MessageEntry const& entry) override;
		void DoFlush() override;
	};
}