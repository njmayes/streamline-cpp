#include "VertexArray.h"

#include <glad/glad.h>

namespace slc {

	static GLenum ShaderDataTypetoOpenGLType( ShaderDataType type )
	{
		switch ( type )
		{
			case ShaderDataType::Float:
			case ShaderDataType::Float2:
			case ShaderDataType::Float3:
			case ShaderDataType::Float4:
			case ShaderDataType::Mat3:
			case ShaderDataType::Mat4:
				return GL_FLOAT;
			case ShaderDataType::Int:
			case ShaderDataType::Int2:
			case ShaderDataType::Int3:
			case ShaderDataType::Int4:
				return GL_INT;
			case ShaderDataType::Bool:
				return GL_BOOL;
		}

		ASSERT( false, "Unknown ShaderDataType!" );
		return 0;
	}

	VertexArray::VertexArray()
	{
		glGenVertexArrays( 1, &mRendererID );
	}

	VertexArray::~VertexArray()
	{
		glDeleteVertexArrays( 1, &mRendererID );
	}

	void VertexArray::Bind() const
	{
		glBindVertexArray( mRendererID );
	}

	void VertexArray::Unbind() const
	{
		glBindVertexArray( 0 );
	}

	void VertexArray::AddVertexBuffer( const Ref< VertexBuffer >& vertexBuffer )
	{
		ASSERT( vertexBuffer->GetLayout().GetElements().size() != 0, "Vertex buffer has no layout!" );

		glBindVertexArray( mRendererID );
		vertexBuffer->Bind();

		const auto& layout = vertexBuffer->GetLayout();
		for ( const auto& element : layout )
		{
			switch ( element.type )
			{
				case ShaderDataType::Float:
				case ShaderDataType::Float2:
				case ShaderDataType::Float3:
				case ShaderDataType::Float4:
				{
					glEnableVertexAttribArray( mVertexBufferIndex );
					glVertexAttribPointer( mVertexBufferIndex, element.GetComponentCount(), ShaderDataTypetoOpenGLType( element.type ), element.normalised ? GL_TRUE : GL_FALSE, layout.GetStride(), ( const void* )element.offset );
					mVertexBufferIndex++;
					break;
				}
				case ShaderDataType::Int:
				case ShaderDataType::Int2:
				case ShaderDataType::Int3:
				case ShaderDataType::Int4:
				case ShaderDataType::Bool:
				{
					glEnableVertexAttribArray( mVertexBufferIndex );
					glVertexAttribIPointer( mVertexBufferIndex, element.GetComponentCount(), ShaderDataTypetoOpenGLType( element.type ), layout.GetStride(), ( const void* )element.offset );
					mVertexBufferIndex++;
					break;
				}
				case ShaderDataType::Mat3:
				case ShaderDataType::Mat4:
				{
					uint8_t count = element.GetComponentCount();
					for ( uint8_t i = 0; i < count; i++ )
					{
						glEnableVertexAttribArray( mVertexBufferIndex );
						glVertexAttribPointer( mVertexBufferIndex, count, ShaderDataTypetoOpenGLType( element.type ), element.normalised ? GL_TRUE : GL_FALSE, layout.GetStride(), ( const void* )( sizeof( float ) * count * i ) );
						glVertexAttribDivisor( mVertexBufferIndex, 1 );
						mVertexBufferIndex++;
					}
					break;
				}
				default:
					ASSERT( false, "Unknown ShaderDataType!" );
			}
		}

		mVertexBuffers.push_back( vertexBuffer );
	}

	void VertexArray::SetIndexBuffer( const Ref< IndexBuffer >& indexBuffer )
	{
		glBindVertexArray( mRendererID );
		indexBuffer->Bind();

		mIndexBuffer = indexBuffer;
	}

} // namespace slc