#include "Timestep.h"

#include <chrono>

namespace sl {
	
	using clock = std::chrono::steady_clock;
	static const auto start = clock::now();

	float Timestep::Now()
	{
		const auto now = clock::now();

		std::chrono::duration< float > elapsed = now - start;
		return elapsed.count();
	}
} // namespace sl
