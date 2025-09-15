#pragma once

#include "slc/Types/Buffer.h"

#include "SharedBuffer.h"

namespace slc::ipc {

	class SharedMemory
	{
	public:
		static SharedMemory Create( std::string_view name, std::size_t size );
		static SharedMemory Acquire( std::string_view name );
		static void Delete( SharedMemory const& memory );

		SharedMemory( SharedMemory const& ) = delete;
		SharedMemory( SharedMemory&& ) = default;

		SharedMemory& operator=( SharedMemory const& ) = delete;
		SharedMemory& operator=( SharedMemory&& ) = default;

		~SharedMemory();

		SharedBuffer Use() const;

	private:
		SharedMemory( std::string_view name, std::size_t size );
		SharedMemory( std::string_view name );

	private:
		struct Impl;
		Box< Impl > mImpl;
	};
} // namespace slc::ipc