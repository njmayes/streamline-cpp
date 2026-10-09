
#pragma once

#include "LogMemoryArena.h"
#include "LoggerStats.h"
#include "Targets/ILogTarget.h"

#include "SL/Core/Common/Profiling.h"

namespace sl {

	struct BenchmarkLogger;

	class Logger
	{
	public:
		static constexpr std::size_t BufferSizeLimit = 512;
		static constexpr std::size_t MessageSizeLimit = BufferSizeLimit - 1;
		static constexpr std::size_t MaxMessagesBeforeFlush = 1024;
		static constexpr std::size_t TemporaryBufferSize = MessageSizeLimit;

		using TemporaryBuffer = std::array< char, BufferSizeLimit >;

		static constexpr Duration MaxTimeBetweenFlush = Duration{ std::chrono::milliseconds( 100 ) };

		struct MessageStorage
		{
			std::vector< MessageEntry > queue;
			LogMemoryArena arena;

			MessageStorage( std::size_t buffer_size_limit, std::size_t max_messages_before_flush )
				: arena( CalculateCapacity( buffer_size_limit, max_messages_before_flush ) )
			{
				queue.reserve( max_messages_before_flush );
			}

			void Reset()
			{
				queue.clear();
				arena.ReleaseBuffers();
			}

		private:
			static std::size_t CalculateCapacity( std::size_t buffer_size_limit, std::size_t max_messages )
			{
				if ( buffer_size_limit < 2 or max_messages == 0 )
					throw std::invalid_argument( "Invalid logger buffer configuration" );

				if ( max_messages > std::numeric_limits< std::size_t >::max() / buffer_size_limit )
					throw std::length_error( "Logger arena capacity overflow" );

				return buffer_size_limit * max_messages;
			}
		};

		struct TimestampCache
		{
			std::chrono::system_clock::time_point timestamp{};
			TemporaryBuffer format_string{};
		};

	private:
		class IndexQueue
		{
		public:
			explicit IndexQueue( std::size_t capacity )
				: mIndices( capacity )
			{
				if ( capacity == 0 )
					throw std::invalid_argument( "IndexQueue capacity must be positive" );
			}

			bool Empty() const
			{
				return mCount == 0;
			}

			std::size_t Size() const
			{
				return mCount;
			}

			void Push( std::size_t index )
			{
				SL_ASSERT( mCount < mIndices.size(), "IndexQueue is full" );

				auto const position = ( mHead + mCount ) % mIndices.size();

				mIndices[ position ] = index;
				++mCount;
			}

			std::size_t Pop()
			{
				SL_ASSERT( not Empty(), "IndexQueue is empty" );

				auto const index = mIndices[ mHead ];

				mHead = ( mHead + 1 ) % mIndices.size();
				--mCount;

				return index;
			}

		private:
			std::vector< std::size_t > mIndices;
			std::size_t mHead{};
			std::size_t mCount{};
		};

	public:
		static Logger& GetGlobalLogger();
		static Logger& GetErrorLogger();

		// Maximum formatted message length is buffer_size_limit - 1.
		Logger(
			std::size_t buffer_size_limit = BufferSizeLimit,
			std::size_t max_messages_before_flush = MaxMessagesBeforeFlush,
			std::size_t buffer_count = 4
		);

		~Logger();

		Logger( Logger const& ) = delete;
		Logger& operator=( Logger const& ) = delete;

		void SetLogLevel( LogLevel level );

		template < typename target_t, typename... Args >
			requires std::derived_from< target_t, ILogTarget > and
					 std::constructible_from< target_t, Args... >
		void AddLogTarget( Args&&... args )
		{
			auto target = MakeBox< target_t >( std::forward< Args >( args )... );

			std::scoped_lock lock( mTargetsMutex );
			mLogTargets.push_back( std::move( target ) );
		}

		void Log( LogLevel level, std::string_view message );

		template < typename... Args >
		void Log( LogLevel level, std::format_string< Args... > message, Args&&... args )
		{
			SL_PROFILE_FUNCTION();

			if ( std::to_underlying( level ) <
				 std::to_underlying( mMinLogLevel.load( std::memory_order_relaxed ) ) )
				return;

			TemporaryBuffer temp{};

			auto const limit = std::min(
				temp.size() - 1,
				mBufferSizeLimit - 1
			);

			auto result = std::format_to_n(
				temp.data(),
				limit,
				message,
				std::forward< Args >( args )...
			);

			auto const size = std::min(
				static_cast< std::size_t >( result.size ),
				limit
			);

			temp[ size ] = '\0';

			Log( level, std::string_view( temp.data(), size ) );
		}

		BenchmarkLogger GetBenchmarkLogger();

	private:
		void ProcessQueue();
		void Flush( MessageStorage& storage );

		MessageStorage& GetStorage( std::size_t index );
		MessageStorage& GetActiveStorage();

		void ActivateFreeStorage();
		void QueueActiveStorage();

		TemporaryBuffer& GetCurrentTimestamp();

	private:
		std::thread mWorker;

		std::mutex mQueueMutex;
		std::condition_variable mWorkerCV;
		std::condition_variable mProducerCV;

		bool mTerminate{ false };

		TimePoint mLastFlush{ Clock::now() };

		std::atomic< LogLevel > mMinLogLevel{ LogLevel::Debug };

		std::size_t mBufferSizeLimit;
		std::size_t mMaxMessagesBeforeFlush;

		std::vector< std::unique_ptr< MessageStorage > > mMessageStorage;

		IndexQueue mFreeBuffers;
		IndexQueue mPendingBuffers;

		std::optional< std::size_t > mActiveBuffer;

		std::mutex mTargetsMutex;
		std::vector< Box< ILogTarget > > mLogTargets;

		LoggerStats mStats;

		friend struct BenchmarkLogger;
	};
} // namespace sl
