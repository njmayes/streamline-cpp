#include "Texture.h"

#include "SL/Core/Logging/Log.h"

#include <glad/gl.h>

#include "stb_image.h"

namespace sl {

	Texture2D::Texture2D( int width, int height )
		: mWidth( width ), mHeight( height )
	{
		mInternalFormat = GL_RGBA8;
		mDataFormat = GL_RGBA;

		glCreateTextures( GL_TEXTURE_2D, 1, &mRendererID );
		glTextureStorage2D( mRendererID, 1, mInternalFormat, mWidth, mHeight );

		glTextureParameteri( mRendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTextureParameteri( mRendererID, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

		glTextureParameteri( mRendererID, GL_TEXTURE_WRAP_S, GL_REPEAT );
		glTextureParameteri( mRendererID, GL_TEXTURE_WRAP_T, GL_REPEAT );
	}

	Texture2D::Texture2D( std::string_view path )
	{
		int width, height, channels;
		stbi_set_flip_vertically_on_load( 1 );
		uint8_t* data = nullptr;
		data = stbi_load( path.data(), &width, &height, &channels, 0 );

		if ( stbi_failure_reason() )
			log::Warn( "{0}", stbi_failure_reason() );
		SL_ASSERT( data, "Failed to load image!" );

		mWidth = width;
		mHeight = height;

		GLenum internal_format = 0, data_format = 0;
		if ( channels == 4 )
		{
			internal_format = GL_RGBA8;
			data_format = GL_RGBA;
		}
		else if ( channels == 3 )
		{
			internal_format = GL_RGB8;
			data_format = GL_RGB;
		}

		mInternalFormat = internal_format;
		mDataFormat = data_format;

		SL_ASSERT( internal_format & data_format, "Format not supported!" );

		glCreateTextures( GL_TEXTURE_2D, 1, &mRendererID );
		glTextureStorage2D( mRendererID, 1, internal_format, mWidth, mHeight );

		glTextureParameteri( mRendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTextureParameteri( mRendererID, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

		glTextureParameteri( mRendererID, GL_TEXTURE_WRAP_S, GL_REPEAT );
		glTextureParameteri( mRendererID, GL_TEXTURE_WRAP_T, GL_REPEAT );

		glTextureSubImage2D( mRendererID, 0, 0, 0, mWidth, mHeight, data_format, GL_UNSIGNED_BYTE, data );

		stbi_image_free( data );
	}

	Texture2D::~Texture2D()
	{
		glDeleteTextures( 1, &mRendererID );
	}

	uint32_t Texture2D::GetSize() const
	{
		uint32_t bpp = mDataFormat == GL_RGBA ? 4 : 3;
		return mWidth * mHeight * bpp;
	}

	void Texture2D::SetData( void* data, size_t size )
	{
		uint32_t bpp = mDataFormat == GL_RGBA ? 4 : 3;
		SL_ASSERT( size == mWidth * mHeight * bpp, "Data must be entire texture!" );
		glTextureSubImage2D( mRendererID, 0, 0, 0, mWidth, mHeight, mDataFormat, GL_UNSIGNED_BYTE, data );
	}

	void Texture2D::SetData( Buffer buffer )
	{
		uint32_t bpp = mDataFormat == GL_RGBA ? 4 : 3;
		SL_ASSERT( buffer.Size() == mWidth * mHeight * bpp, "Data must be entire texture!" );
		glTextureSubImage2D( mRendererID, 0, 0, 0, mWidth, mHeight, mDataFormat, GL_UNSIGNED_BYTE, buffer.Data() );
	}

	Buffer Texture2D::GetData()
	{
		uint32_t bpp = mDataFormat == GL_RGBA ? 4 : 3;
		int size = mWidth * mHeight * bpp;

		Buffer buffer( size );
		glGetTextureImage( mRendererID, GL_TEXTURE_2D, mDataFormat, GL_UNSIGNED_BYTE, size, buffer.Data() );
		return buffer;
	}
} // namespace sl