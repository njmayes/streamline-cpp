#pragma once

#include "Timer.h"

namespace slc {

	class ScopedTimer
	{
	public:
		ScopedTimer(std::string_view name) : m_Name(name) {}
		~ScopedTimer()
		{
			float time = mTimer.ElapsedMillis();
			std::cout << mName << " - " << time << "ms\n";
		}
	private:
		Timer mTimer;
		std::string mName;
	};

}