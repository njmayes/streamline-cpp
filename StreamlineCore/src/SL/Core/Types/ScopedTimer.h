#pragma once

#include "Timer.h"

#include <print>

namespace sl {

	class ScopedTimer
	{
	public:
		ScopedTimer( std::string_view name )
			: mName( name )
		{}
		~ScopedTimer()
		{
			float time = mTimer.ElapsedMillis();
			std::println( "{} - {}ms", mName, time );
		}

	private:
		Timer mTimer;
		std::string mName;
	};

} // namespace sl