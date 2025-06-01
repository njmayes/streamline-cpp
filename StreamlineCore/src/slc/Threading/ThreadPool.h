#pragma once

#include <condition_variable>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <functional>
#include <type_traits>

namespace slc {

	template < typename Function, typename TReturn, typename... Args >
	concept InvokeReturnConvertibleTo = std::same_as< std::invoke_result_t< Function, Args&&... >, TReturn >;

	template < typename Function, typename... Args >
	concept ReturnValuedFunction = std::invocable< Function, Args... > and not InvokeReturnConvertibleTo< Function, void, Args... >;

	template < typename Function, typename... Args >
	concept VoidFunction = std::invocable< Function, Args... > and InvokeReturnConvertibleTo< Function, void, Args... >;

	class ThreadPool
	{
	public:
		ThreadPool( size_t num_threads = std::thread::hardware_concurrency() );

		~ThreadPool();

		ThreadPool( const ThreadPool& ) = delete;
		ThreadPool( ThreadPool&& ) = default;

		ThreadPool& operator=( const ThreadPool& ) = delete;
		ThreadPool& operator=( ThreadPool&& ) = default;

		// Enqueue task that returns a value for execution by the thread pool
		template < typename Function, typename... Args >
			requires ReturnValuedFunction< Function, Args... >
		auto Queue( Function&& f, Args&&... args ) -> std::future< std::invoke_result_t< Function, Args&&... > >
		{
			using ReturnType = std::invoke_result_t< Function, Args&&... >;
			std::promise< ReturnType > return_promise;

			auto future = return_promise.get_future();

			auto task = [ ret = std::move( return_promise ), job = std::move( f ), ... args = std::forward< Args >( args ) ]() mutable {
				ret.set_value( std::invoke( job, std::forward< Args >( args )... ) );
			};

			{
				std::unique_lock< std::mutex > lock( mQueueMutex );
				mTasks.emplace( std::move( task ) );
			}
			mCV.notify_one();

			return future;
		}

		// Enqueue task for execution by the thread pool
		template < typename Function, typename... Args >
			requires VoidFunction< Function, Args... >
		auto Queue( Function&& f, Args&&... args ) -> void
		{
			auto task = [ job = std::move( f ), ... args = std::forward< Args >( args ) ]() mutable {
				std::invoke( job, std::forward< Args >( args )... );
			};

			{
				std::unique_lock< std::mutex > lock( mQueueMutex );
				mTasks.emplace( std::move( task ) );
			}
			mCV.notify_one();
		}

	private:
		std::vector< std::thread > mThreads;
		std::queue< std::move_only_function< void() > > mTasks;
		std::mutex mQueueMutex;
		std::condition_variable mCV;
		bool mStop = false;
	};
} // namespace slc