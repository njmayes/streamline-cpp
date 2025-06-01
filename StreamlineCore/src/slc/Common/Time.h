#pragma once

#include <ctime>

namespace slc {

	inline std::tm GetLocalTime( std::time_t const* timer )
	{
		std::tm bt{};
#if defined( SLC_PLATFORM_LINUX )
		localtime_r( &timer, &bt );
#elif defined( SLC_PLATFORM_WINDOWS )
		localtime_s( &bt, timer );
#else
		static std::mutex mtx;
		std::lock_guard< std::mutex > lock( mtx );
		bt = *std::localtime( &timer );
#endif
		return bt;
	}
} // namespace slc