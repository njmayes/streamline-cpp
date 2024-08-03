#pragma once

#include <chrono>

namespace slc {

	class Timer
	{
	public:
		Timer()
		{
			reset();
		}

		void reset()
		{
			mStart = std::chrono::steady_clock::now();
		}

		float elapsed()
		{
			return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - mStart).count() * 0.001f * 0.001f * 0.001f;
		}

		float elapsedMillis()
		{
			return elapsed() * 1000.0f;
		}

	private:
		std::chrono::time_point<std::chrono::steady_clock> mStart;
	};

}