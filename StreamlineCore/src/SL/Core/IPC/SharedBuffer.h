#pragma once

#include "SL/Core/Types/Buffer.h"

#include "SharedMutex.h"

namespace sl::ipc {

	// A RAII buffer object that locks and unlocks the shared mutex for the associated shared memory.
	class SharedBuffer : public BufferView
	{
	public:
		SharedBuffer( std::string_view name, BufferView buffer );
		virtual ~SharedBuffer();

		SharedBuffer( SharedBuffer const& ) = delete;
		SharedBuffer( SharedBuffer&& ) = default;

		SharedBuffer& operator=( SharedBuffer const& ) = delete;
		SharedBuffer& operator=( SharedBuffer&& ) = default;

	private:
		SharedMutex mMutex;
	};
}