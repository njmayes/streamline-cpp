#include "SharedBuffer.h"

namespace sl::ipc {

	SharedBuffer::SharedBuffer( std::string_view name, BufferView buffer )
		: BufferView( buffer )
		, mMutex( name )
	{
		if ( not mMutex.IsValid() )
			throw std::runtime_error( "Could not create or acquire mutex for this shared memory" );

		mMutex.Lock();
	}

	SharedBuffer::~SharedBuffer()
	{
		mMutex.Unlock();
	}
} // namespace sl::ipc