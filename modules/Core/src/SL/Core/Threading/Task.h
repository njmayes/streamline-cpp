#pragma once

#include "TaskState.h"

namespace sl {
	class ThreadPool;

	template < typename T >
	class Task
	{
	public:
		Task() = default;
		Task( Task&& ) = default;
		Task& operator=( Task&& ) = default;
		Task( const Task& ) = delete;
		Task& operator=( const Task& ) = delete;

		[[nodiscard]] bool Valid() const noexcept
		{
			return static_cast< bool >( state_ );
		}
		decltype( auto ) Get()
		{
			if ( !state_ )
				throw std::future_error( std::future_errc::no_state );
			return std::exchange( state_, {} )->future.get();
		}
		auto operator co_await() &&
		{
			if ( !state_ )
				throw std::future_error( std::future_errc::no_state );
			return thread::FutureAwaiter< T >{ std::exchange( state_, {} ) };
		}
		template < typename F >
		auto Then( F&& fn ) && -> Task< thread::ContinuationResultT< T, F > >;

	private:
		friend class ThreadPool;
		explicit Task( ThreadPool* pool, std::shared_ptr< thread::State< T > > state ) : pool_( pool ), state_( std::move( state ) )
		{}
		ThreadPool* pool_ = nullptr;
		std::shared_ptr< thread::State< T > > state_;
	};
} // namespace sl
