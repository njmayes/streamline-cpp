#include "ShadowMap3D.h"

#include <glad/gl.h>
#include <stdexcept>

namespace sl {

	ShadowMap3D::ShadowMap3D( uint32_t resolution ) { Resize( resolution ); }

	ShadowMap3D::~ShadowMap3D()
	{
		glDeleteTextures( 1, &mTexture );
		glDeleteFramebuffers( 1, &mFramebuffer );
	}

	void ShadowMap3D::Resize( uint32_t resolution )
	{
		if ( !resolution || resolution == mResolution ) return;
		if ( !mFramebuffer ) glCreateFramebuffers( 1, &mFramebuffer );
		if ( mTexture ) glDeleteTextures( 1, &mTexture );
		mResolution = resolution;
		glCreateTextures( GL_TEXTURE_2D, 1, &mTexture );
		glTextureStorage2D( mTexture, 1, GL_DEPTH_COMPONENT32F, resolution, resolution );
		glTextureParameteri( mTexture, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTextureParameteri( mTexture, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTextureParameteri( mTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER );
		glTextureParameteri( mTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER );
		const float border[] = { 1, 1, 1, 1 };
		glTextureParameterfv( mTexture, GL_TEXTURE_BORDER_COLOR, border );
		glTextureParameteri( mTexture, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE );
		glTextureParameteri( mTexture, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL );
		glNamedFramebufferTexture( mFramebuffer, GL_DEPTH_ATTACHMENT, mTexture, 0 );
		glNamedFramebufferDrawBuffer( mFramebuffer, GL_NONE );
		glNamedFramebufferReadBuffer( mFramebuffer, GL_NONE );
		if ( glCheckNamedFramebufferStatus( mFramebuffer, GL_FRAMEBUFFER ) != GL_FRAMEBUFFER_COMPLETE )
			throw std::runtime_error( "Shadow framebuffer incomplete" );
	}

	void ShadowMap3D::Bind() const
	{
		glBindFramebuffer( GL_FRAMEBUFFER, mFramebuffer );
		glViewport( 0, 0, static_cast< GLsizei >( mResolution ), static_cast< GLsizei >( mResolution ) );
	}

} // namespace sl
