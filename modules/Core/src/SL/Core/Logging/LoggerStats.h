#pragma once

#include "SL/Core/Common/Base.h"

namespace sl {

	class LoggerStats
	{
	public:
		struct Data
		{
			std::size_t total_flushes{};
			std::size_t messages_since_last_flush{};
			std::chrono::duration< float, std::micro > time_since_last_flush{};
			std::size_t large_message_count{};
		};

		void AddMessage( std::size_t message_size, std::size_t size_limit )
		{
			Update( [ = ]( auto& data ) {
				if ( message_size == size_limit )
					data.large_message_count++;

				data.messages_since_last_flush++;
			} );
		}

		void Reset()
		{
			Update( []( auto& data ) {
				data.messages_since_last_flush = 0;
				data.large_message_count = 0;
			} );
		}

		void Flush( TimePoint& last_flush )
		{
			Update( [ & ]( auto& data ) {
				data.total_flushes++;
				data.large_message_count = 0;
				data.messages_since_last_flush = 0;

				auto flush_time = Clock::now();
				data.time_since_last_flush = flush_time - last_flush;
				last_flush = flush_time;
			} );
		}

	private:
		void Update( IsAction< Data& > auto&& edit )
		{
			std::scoped_lock lock( mStatsMutex );
			edit( mData );
		}

	private:
		friend struct BenchmarkLogger;

		std::mutex mStatsMutex;
		Data mData;
	};
} // namespace sl