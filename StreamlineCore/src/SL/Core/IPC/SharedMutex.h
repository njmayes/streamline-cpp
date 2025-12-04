#pragma once

#include "SL/Core/Common/Base.h"

namespace sl::ipc {

	class SharedMutex
	{
	public:
		SharedMutex( std::string_view name );

		SharedMutex( SharedMutex const& ) = delete;
		SharedMutex( SharedMutex&& ) noexcept;

		SharedMutex& operator=( SharedMutex const& ) = delete;
		SharedMutex& operator=( SharedMutex&& ) noexcept;

		~SharedMutex();

		void Lock();
		void Unlock();

		bool IsValid() const;

	private:
		struct Impl;
		Box< Impl > mImpl;
	};
} // namespace sl::ipc