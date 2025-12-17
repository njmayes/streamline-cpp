#pragma once

#include "SL/Core/Types/Buffer.h"

#include "SharedBuffer.h"

namespace sl::ipc {

	class SharedMemory
	{
	public:
		static SharedMemory Create( std::string_view name, std::size_t size );
		static SharedMemory Acquire( std::string_view name );

		SharedMemory( SharedMemory const& ) = delete;
		SharedMemory( SharedMemory&& ) = default;

		SharedMemory& operator=( SharedMemory const& ) = delete;
		SharedMemory& operator=( SharedMemory&& ) = default;

		~SharedMemory();

		SharedBuffer View();
		BufferView const& View() const;

	private:
		SharedMemory( std::string_view name, std::size_t size );
		SharedMemory( std::string_view name );

	private:
		struct Impl;
		Box< Impl > mImpl;
	};
} // namespace sl::ipc