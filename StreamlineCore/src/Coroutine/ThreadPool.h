#pragma once

#include "Collections/Deque.h"
#include "Collections/Vector.h"

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