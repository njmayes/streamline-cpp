#pragma once

#include "slc/Common/Base.h"

namespace slc::ipc {

	class SharedMutex
	{
	public:
		SharedMutex( std::string_view name, bool create );

		SharedMutex( SharedMutex const& ) = delete;
		SharedMutex( SharedMutex&& ) = default;

		SharedMutex& operator=( SharedMutex const& ) = delete;
		SharedMutex& operator=( SharedMutex&& ) = default;

		~SharedMutex();

		void Lock();
		void Unlock();

		bool IsValid() const;

	private:
		struct Impl;
		Box< Impl > mImpl;
	};
} // namespace slc::ipc