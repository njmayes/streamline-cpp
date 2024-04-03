#pragma once

#include "Containers/Deque.h"
#include "Containers/Vector.h"

#include "Task.h"

#include <thread>
#include <deque>

namespace slc {

	class ThreadPool
	{
	public:

	private:
		Vector<std::thread> mThreads;

		Deque<std::coroutine_handle<>> mQueue;
	};
}