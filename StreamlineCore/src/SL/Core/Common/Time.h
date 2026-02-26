#pragma once

#include <ctime>

namespace sl {

	inline std::tm GetLocalTime( std::time_t const* timer )
	{
		std::tm bt{};
#if defined( SL_PLATFORM_LINUX )
		localtime_r( timer, &bt );
#elif defined( SL_PLATFORM_WINDOWS )
		localtime_s( &bt, timer );
#else
		static std::mutex mtx;
		std::lock_guard< std::mutex > lock( mtx );
		bt = *std::localtime( &timer );
#endif
		return bt;
	}
} // namespace sl