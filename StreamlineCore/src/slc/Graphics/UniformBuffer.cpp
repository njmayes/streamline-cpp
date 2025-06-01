#include "UniformBuffer.h"

#include <glad/glad.h>

namespace slc {

	UniformBuffer::UniformBuffer( uint32_t size, uint32_t binding )
	{
		glCreateBuffers( 1, &mRendererID );
		glNamedBufferData( mRendererID, size, nullptr, GL_DYNAMIC_DRAW );
		glBindBufferBase( GL_UNIFORM_BUFFER, binding, mRendererID );
	}

	UniformBuffer::~UniformBuffer()
	{
		glDeleteBuffers( 1, &mRendererID );
	}

	void UniformBuffer::SetData( const void* data, uint32_t size, uint32_t offset )
	{
		glNamedBufferSubData( mRendererID, offset, size, data );
	}
} // namespace slc
