#include "Framebuffer.h"

#include <glad/glad.h>

namespace slc {

	namespace detail {

		static GLenum TextureTarget( bool multisampled )
		{
			return multisampled ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
		}

		static void CreateTextures( bool multisampled, uint32_t* outID, int count )
		{
			glCreateTextures( TextureTarget( multisampled ), count, outID );
		}

		static void BindTexture( bool multisampled, uint32_t id )
		{
			glBindTexture( TextureTarget( multisampled ), id );
		}

		static void AttachColorTexture( uint32_t id, int samples, GLenum internalFormat, GLenum format, int width, int height, int index )
		{
			bool multisampled = samples > 1;
			if ( multisampled )
			{
				glTexImage2DMultisample( GL_TEXTURE_2D_MULTISAMPLE, samples, internalFormat, width, height, GL_FALSE );
			}
			else
			{
				glTexImage2D( GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, nullptr );

				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
			}

			glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, TextureTarget( multisampled ), id, 0 );
		}

		static void AttachDepthTexture( uint32_t id, int samples, GLenum format, GLenum attachmentType, int width, int height )
		{
			bool multisampled = samples > 1;
			if ( multisampled )
			{
				glTexImage2DMultisample( GL_TEXTURE_2D_MULTISAMPLE, samples, format, width, height, GL_FALSE );
			}
			else
			{
				glTexStorage2D( GL_TEXTURE_2D, 1, format, width, height );

				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
			}

			glFramebufferTexture2D( GL_FRAMEBUFFER, attachmentType, TextureTarget( multisampled ), id, 0 );
		}

		static bool IsDepthFormat( FramebufferTextureFormat format )
		{
			switch ( format )
			{
				case FramebufferTextureFormat::DEPTH24STENCIL8:
					return true;
			}

			return false;
		}

		static GLenum LabyrinthFBTextureFormatToGL( FramebufferTextureFormat format )
		{
			switch ( format )
			{
				case FramebufferTextureFormat::RGBA8:
					return GL_RGBA8;
				case FramebufferTextureFormat::RED_INTEGER:
					return GL_RED_INTEGER;
			}

			ASSERT( false );
			return 0;
		}
	} // namespace detail

	Framebuffer::Framebuffer( const FramebufferSpec& spec )
		: mSpecification( spec )
	{
		for ( auto spec : mSpecification.attachments.attachments )
		{
			if ( !detail::IsDepthFormat( spec.texture_format ) )
				mColourAttachmentSpecs.emplace_back( spec );
			else
				mDepthAttachmentSpec = spec;
		}

		Invalidate();
	}

	Framebuffer::~Framebuffer()
	{
		glDeleteFramebuffers( 1, &mRendererID );
		glDeleteTextures( ( int )mColourAttachments.size(), mColourAttachments.data() );
		glDeleteTextures( 1, &mDepthAttachment );
	}

	void Framebuffer::Invalidate()
	{
		if ( mRendererID )
		{
			glDeleteFramebuffers( 1, &mRendererID );
			glDeleteTextures( ( int )mColourAttachments.size(), mColourAttachments.data() );
			glDeleteTextures( 1, &mDepthAttachment );

			mColourAttachments.clear();
			mDepthAttachment = 0;
		}

		glCreateFramebuffers( 1, &mRendererID );
		glBindFramebuffer( GL_FRAMEBUFFER, mRendererID );

		bool multisample = mSpecification.samples > 1;

		// Attachments
		if ( mColourAttachmentSpecs.size() )
		{
			mColourAttachments.resize( mColourAttachmentSpecs.size() );
			detail::CreateTextures( multisample, mColourAttachments.data(), ( int )mColourAttachments.size() );

			for ( size_t i = 0; i < mColourAttachments.size(); i++ )
			{
				detail::BindTexture( multisample, mColourAttachments[ i ] );
				switch ( mColourAttachmentSpecs[ i ].texture_format )
				{
					case FramebufferTextureFormat::RGBA8:
						detail::AttachColorTexture( mColourAttachments[ i ], mSpecification.samples, GL_RGBA8, GL_RGBA, mSpecification.width, mSpecification.height, ( int )i );
						break;
					case FramebufferTextureFormat::RED_INTEGER:
						detail::AttachColorTexture( mColourAttachments[ i ], mSpecification.samples, GL_R32I, GL_RED_INTEGER, mSpecification.width, mSpecification.height, ( int )i );
						break;
				}
			}
		}

		if ( mDepthAttachmentSpec.texture_format != FramebufferTextureFormat::None )
		{
			detail::CreateTextures( multisample, &mDepthAttachment, 1 );
			detail::BindTexture( multisample, mDepthAttachment );
			switch ( mDepthAttachmentSpec.texture_format )
			{
				case FramebufferTextureFormat::DEPTH24STENCIL8:
					detail::AttachDepthTexture( mDepthAttachment, mSpecification.samples, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL_ATTACHMENT, mSpecification.width, mSpecification.height );
					break;
			}
		}

		if ( mColourAttachments.size() > 1 )
		{
			ASSERT( mColourAttachments.size() <= 4 );
			GLenum buffers[ 4 ] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
			glDrawBuffers( ( int )mColourAttachments.size(), buffers );
		}
		else if ( mColourAttachments.empty() )
		{
			// Only depth-pass
			glDrawBuffer( GL_NONE );
		}

		ASSERT( glCheckFramebufferStatus( GL_FRAMEBUFFER ) == GL_FRAMEBUFFER_COMPLETE, "Framebuffer is incomplete!" );

		glBindFramebuffer( GL_FRAMEBUFFER, 0 );
	}

	void Framebuffer::Bind()
	{
		glBindFramebuffer( GL_FRAMEBUFFER, mRendererID );
		glViewport( 0, 0, mSpecification.width, mSpecification.height );
	}

	void Framebuffer::Unbind()
	{
		glBindFramebuffer( GL_FRAMEBUFFER, 0 );
	}

	void Framebuffer::Resize( size_t width, size_t height )
	{
		mSpecification.width = ( int )width;
		mSpecification.height = ( int )height;

		Invalidate();
	}

	int Framebuffer::ReadPixel( uint32_t attachmentIndex, int x, int y ) const
	{
		ASSERT( attachmentIndex < mColourAttachments.size() );

		glReadBuffer( GL_COLOR_ATTACHMENT0 + attachmentIndex );
		int pixelData;
		glReadPixels( x, y, 1, 1, GL_RED_INTEGER, GL_INT, &pixelData );
		return pixelData;
	}

	void Framebuffer::ClearAttachment( uint32_t attachmentIndex, int value )
	{
		ASSERT( attachmentIndex < mColourAttachments.size() );

		auto& spec = mColourAttachmentSpecs[ attachmentIndex ];
		glClearTexImage( mColourAttachments[ attachmentIndex ], 0, detail::LabyrinthFBTextureFormatToGL( spec.texture_format ), GL_INT, &value );
	}

	void Framebuffer::BindColourAttachment( uint32_t index ) const
	{
		ASSERT( index < mColourAttachments.size(), "Binding attachment out of range!" );

		bool multisample = mSpecification.samples > 1;
		detail::BindTexture( multisample, mColourAttachments[ index ] );
	}
} // namespace slc