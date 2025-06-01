#include "Buffer.h"

#include "glad/glad.h"

namespace slc {

	///
	/// OpenGL Vertex Buffer
	///

	VertexBuffer::VertexBuffer( uint32_t size )
	{
		glGenBuffers( 1, &mRendererID );
		glBindBuffer( GL_ARRAY_BUFFER, mRendererID );
		glBufferData( GL_ARRAY_BUFFER, size, nullptr, GL_DYNAMIC_DRAW );
	}

	VertexBuffer::VertexBuffer( float* vertices, uint32_t size )
	{
		glGenBuffers( 1, &mRendererID );
		glBindBuffer( GL_ARRAY_BUFFER, mRendererID );
		glBufferData( GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW );
	}

	VertexBuffer::~VertexBuffer()
	{
		glDeleteBuffers( 1, &mRendererID );
	}

	void VertexBuffer::Bind() const
	{
		glBindBuffer( GL_ARRAY_BUFFER, mRendererID );
	}

	void VertexBuffer::Unbind() const
	{
		glBindBuffer( GL_ARRAY_BUFFER, 0 );
	}

	void VertexBuffer::SetData( const void* data, uint32_t size )
	{
		glBindBuffer( GL_ARRAY_BUFFER, mRendererID );
		glBufferSubData( GL_ARRAY_BUFFER, 0, size, data );
	}


	///
	/// OpenGL Index Buffer
	///

	IndexBuffer::IndexBuffer( uint32_t* indices, uint32_t count )
		: mCount( count )
	{
		// GL_ELEMENT_ARRAY_BUFFER is not valid without an actively bound VAO
		// Binding with GL_ARRAY_BUFFER allows the data to be loaded regardless of VAO state.
		glGenBuffers( 1, &mRendererID );
		glBindBuffer( GL_ARRAY_BUFFER, mRendererID );
		glBufferData( GL_ARRAY_BUFFER, count * sizeof( uint32_t ), indices, GL_STATIC_DRAW );
	}

	IndexBuffer::~IndexBuffer()
	{
		glDeleteBuffers( 1, &mRendererID );
	}

	void IndexBuffer::Bind() const
	{
		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, mRendererID );
	}

	void IndexBuffer::Unbind() const
	{
		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
	}
} // namespace slc