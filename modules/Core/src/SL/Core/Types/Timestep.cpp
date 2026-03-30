#include "Timestep.h"

#include <chrono>

namespace sl {
	
	using clock = std::chrono::steady_clock;
	using duration = std::chrono::duration< double >;

	static const auto start = clock::now();

	double Timestep::Now()
	{
		const auto now = clock::now();

		duration elapsed = now - start;
		return elapsed.count();
	}
} // namespace sl
