#include "HDRTarget3D.h"

#include <glad/gl.h>
#include <stdexcept>

namespace sl {

	HDRTarget3D::HDRTarget3D( uint32_t width, uint32_t height ) { Resize( width, height ); }

	HDRTarget3D::~HDRTarget3D()
	{
		glDeleteTextures( 1, &mColour );
		glDeleteRenderbuffers( 1, &mDepthStencil );
		glDeleteFramebuffers( 1, &mFramebuffer );
	}

	void HDRTarget3D::Resize( uint32_t width, uint32_t height )
	{
		if ( !width || !height || ( width == mWidth && height == mHeight ) ) return;
		if ( !mFramebuffer ) glCreateFramebuffers( 1, &mFramebuffer );
		if ( mColour ) glDeleteTextures( 1, &mColour );
		if ( mDepthStencil ) glDeleteRenderbuffers( 1, &mDepthStencil );
		mWidth = width; mHeight = height;
		glCreateTextures( GL_TEXTURE_2D, 1, &mColour );
		glTextureStorage2D( mColour, 1, GL_RGBA16F, width, height );
		glTextureParameteri( mColour, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTextureParameteri( mColour, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTextureParameteri( mColour, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		glTextureParameteri( mColour, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
		glNamedFramebufferTexture( mFramebuffer, GL_COLOR_ATTACHMENT0, mColour, 0 );
		glCreateRenderbuffers( 1, &mDepthStencil );
		glNamedRenderbufferStorage( mDepthStencil, GL_DEPTH24_STENCIL8, width, height );
		glNamedFramebufferRenderbuffer( mFramebuffer, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, mDepthStencil );
		if ( glCheckNamedFramebufferStatus( mFramebuffer, GL_FRAMEBUFFER ) != GL_FRAMEBUFFER_COMPLETE )
			throw std::runtime_error( "HDR framebuffer incomplete" );
	}

	void HDRTarget3D::Bind() const
	{
		glBindFramebuffer( GL_FRAMEBUFFER, mFramebuffer );
		glViewport( 0, 0, static_cast< GLsizei >( mWidth ), static_cast< GLsizei >( mHeight ) );
	}

} // namespace sl
