#pragma once

#include "slc/Types/Buffer.h"

#include "SharedBuffer.h"

namespace slc::ipc {

	class SharedMemory
	{
	public:
		SharedMemory( std::string_view name, std::size_t size, bool create = false );

		SharedMemory( SharedMemory const& ) = delete;
		SharedMemory( SharedMemory&& ) = default;

		SharedMemory& operator=( SharedMemory const& ) = delete;
		SharedMemory& operator=( SharedMemory&& ) = default;

		~SharedMemory();

		SharedBuffer Get() const;

	private:
		struct Impl;
		Box< Impl > mImpl;
	};
} // namespace slc::ipc