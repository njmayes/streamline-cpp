#pragma once

#include <coroutine>
#include <functional>
#include <stdexcept>
#include <utility>

namespace sl::thread {

	// The scheduler is owned by ThreadPool. Its lifetime must cover submitted work.
	struct CoroutineContext
	{
		std::function< void( std::coroutine_handle<> ) > schedule;
	};

	inline thread_local const CoroutineContext* currentContext = nullptr;

	struct ContextScope
	{
		const CoroutineContext* previous;
		explicit ContextScope( const CoroutineContext* context ) noexcept : previous( std::exchange( currentContext, context ) )
		{}
		~ContextScope()
		{
			currentContext = previous;
		}
	};

	inline void Resume( std::coroutine_handle<> handle, const CoroutineContext* context )
	{
		ContextScope scope( context );
		handle.resume();
	}

	inline const CoroutineContext& CurrentContext()
	{
		if ( !currentContext )
			throw std::logic_error( "No coroutine context: submit the coroutine using ThreadPool::CoSubmit" );
		return *currentContext;
	}

} // namespace sl::thread
