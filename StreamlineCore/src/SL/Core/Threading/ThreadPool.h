#pragma once

#include <condition_variable>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <functional>
#include <type_traits>

namespace sl {

	class ThreadPool
	{
	public:
		ThreadPool( size_t num_threads = std::thread::hardware_concurrency() );

		~ThreadPool();

		ThreadPool( const ThreadPool& ) = delete;
		ThreadPool( ThreadPool&& ) = default;

		ThreadPool& operator=( const ThreadPool& ) = delete;
		ThreadPool& operator=( ThreadPool&& ) = default;

		void Shutdown();

		// Enqueue task for execution by the thread pool
		template < typename Function, typename... Args >
		auto Queue( Function&& f, Args&&... args ) -> std::future< std::invoke_result_t< Function, Args&&... > >
		{
			using ReturnType = std::invoke_result_t< Function, Args&&... >;
			std::promise< ReturnType > return_promise;

			auto future = return_promise.get_future();

			auto task = [ ret = std::move( return_promise ), job = std::move( f ), ... args = std::forward< Args >( args ) ]() mutable {
				if constexpr ( std::is_void_v< ReturnType > )
				{
					std::invoke( job, std::forward< Args >( args )... );
					ret.set_value();
				}
				else
				{
					ret.set_value( std::invoke( job, std::forward< Args >( args )... ) );
				}
			};

			{
				std::unique_lock< std::mutex > lock( mQueueMutex );
				mTasks.emplace( std::move( task ) );
			}
			mCV.notify_one();

			return future;
		}

	private:
		std::vector< std::thread > mThreads;
		std::queue< std::move_only_function< void() > > mTasks;
		std::mutex mQueueMutex;
		std::condition_variable mCV;
		bool mStop = false;
	};
} // namespace sl