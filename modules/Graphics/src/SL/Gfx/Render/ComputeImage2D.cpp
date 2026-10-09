#include "ComputeImage2D.h"

#include <glad/gl.h>
#include <cassert>

namespace sl {

	unsigned int ComputeImage2D::GLFormat() const
	{
		switch ( mFormat ) {
			case Format::R32Float: return GL_R32F;
			case Format::RG32Float: return GL_RG32F;
			case Format::R32Unsigned: return GL_R32UI;
			case Format::RGBA8: return GL_RGBA8;
		}
		return GL_RGBA8;
	}

	unsigned int ComputeImage2D::GLDataFormat() const
	{
		switch ( mFormat ) {
			case Format::R32Float: return GL_RED;
			case Format::RG32Float: return GL_RG;
			case Format::R32Unsigned: return GL_RED_INTEGER;
			case Format::RGBA8: return GL_RGBA;
		}
		return GL_RGBA;
	}

	unsigned int ComputeImage2D::GLDataType() const
	{
		return mFormat == Format::R32Unsigned ? GL_UNSIGNED_INT :
			mFormat == Format::RGBA8 ? GL_UNSIGNED_BYTE : GL_FLOAT;
	}

	std::size_t ComputeImage2D::GetPixelSize() const
	{
		return mFormat == Format::RG32Float ? 2 * sizeof( float ) : sizeof( std::uint32_t );
	}

	std::size_t ComputeImage2D::GetByteSize() const
	{
		return static_cast< std::size_t >( mWidth ) * mHeight * GetPixelSize();
	}

	ComputeImage2D::ComputeImage2D( int width, int height, Format format )
		: mWidth( width ), mHeight( height ), mFormat( format )
	{
		assert( width > 0 && height > 0 );
		glCreateTextures( GL_TEXTURE_2D, 1, &mTexture );
		glTextureStorage2D( mTexture, 1, GLFormat(), width, height );
		glTextureParameteri( mTexture, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		glTextureParameteri( mTexture, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
		glTextureParameteri( mTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		glTextureParameteri( mTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
		Clear();
	}

	ComputeImage2D::~ComputeImage2D()
	{
		if ( mTexture ) glDeleteTextures( 1, &mTexture );
	}

	void ComputeImage2D::BindImage( std::uint32_t unit, Access access ) const
	{
		GLenum gl_access = access == Access::Read ? GL_READ_ONLY :
			access == Access::Write ? GL_WRITE_ONLY : GL_READ_WRITE;
		glBindImageTexture( unit, mTexture, 0, GL_FALSE, 0, gl_access, GLFormat() );
	}

	void ComputeImage2D::Upload( void const* data, std::size_t size )
	{
		assert( data && size == GetByteSize() );
		GLint previous = 0;
		glGetIntegerv( GL_PIXEL_UNPACK_BUFFER_BINDING, &previous );
		glBindBuffer( GL_PIXEL_UNPACK_BUFFER, 0 );
		glTextureSubImage2D( mTexture, 0, 0, 0, mWidth, mHeight, GLDataFormat(), GLDataType(), data );
		glBindBuffer( GL_PIXEL_UNPACK_BUFFER, static_cast< GLuint >( previous ) );
	}

	void ComputeImage2D::Download( void* data, std::size_t size ) const
	{
		assert( data && size == GetByteSize() );
		GLint previous = 0;
		glGetIntegerv( GL_PIXEL_PACK_BUFFER_BINDING, &previous );
		glBindBuffer( GL_PIXEL_PACK_BUFFER, 0 );
		glGetTextureImage( mTexture, 0, GLDataFormat(), GLDataType(), static_cast< GLsizei >( size ), data );
		glBindBuffer( GL_PIXEL_PACK_BUFFER, static_cast< GLuint >( previous ) );
	}

	void ComputeImage2D::UploadPixel( int x, int y, void const* data )
	{
		assert( x >= 0 && x < mWidth && y >= 0 && y < mHeight && data );
		GLint previous = 0;
		glGetIntegerv( GL_PIXEL_UNPACK_BUFFER_BINDING, &previous );
		glBindBuffer( GL_PIXEL_UNPACK_BUFFER, 0 );
		glTextureSubImage2D( mTexture, 0, x, y, 1, 1, GLDataFormat(), GLDataType(), data );
		glBindBuffer( GL_PIXEL_UNPACK_BUFFER, static_cast< GLuint >( previous ) );
	}

	void ComputeImage2D::Clear()
	{
		std::uint32_t const zero[ 2 ] = {};
		glClearTexImage( mTexture, 0, GLDataFormat(), GLDataType(), zero );
	}

} // namespace sl
