#pragma once

#include <chrono>

namespace slc {

	class Timer
	{
	public:
		Timer()
		{
			Reset();
		}

		void Reset()
		{
			mStart = std::chrono::steady_clock::now();
		}

		float Elapsed()
		{
			return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - mStart).count() * 0.001f * 0.001f * 0.001f;
		}

		float ElapsedMillis()
		{
			return Elapsed() * 1000.0f;
		}

		float ElapsedMicros()
		{
			return ElapsedMillis() * 1000.0f;
		}

	private:
		std::chrono::time_point<std::chrono::steady_clock> mStart;
	};

}