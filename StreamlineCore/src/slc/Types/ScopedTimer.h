#pragma once

#include "Timer.h"

#include <iostream>

namespace slc {

	class ScopedTimer
	{
	public:
		ScopedTimer(std::string_view name) : mName(name) {}
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