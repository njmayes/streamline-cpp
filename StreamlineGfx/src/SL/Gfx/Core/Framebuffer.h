#pragma once

#include "IRenderable.h"

namespace slc {

	enum class FramebufferTextureFormat
	{
		None = 0,

		// Colour
		RGBA8,
		RED_INTEGER,

		// Depth/stencil
		DEPTH24STENCIL8,

		// Defaults
		Depth = DEPTH24STENCIL8
	};

	struct FramebufferTextureSpec
	{
		FramebufferTextureSpec() = default;
		FramebufferTextureSpec( FramebufferTextureFormat format )
			: texture_format( format )
		{}

		FramebufferTextureFormat texture_format = FramebufferTextureFormat::None;
		// TODO: filtering/wrap
	};

	struct FramebufferAttachmentSpec
	{
		FramebufferAttachmentSpec() = default;
		FramebufferAttachmentSpec( std::initializer_list< FramebufferTextureSpec > attachment_list )
			: attachments( attachment_list )
		{}

		std::vector< FramebufferTextureSpec > attachments;
	};

	struct FramebufferSpec
	{
		int width, height;
		FramebufferAttachmentSpec attachments;
		uint32_t samples = 1;

		bool swapChainTarget = true;
	};

	class Framebuffer : public IRenderable
	{
	public:
		Framebuffer( const FramebufferSpec& spec );
		~Framebuffer();

		void Bind();
		void Unbind();

		uint32_t GetTextureID() const
		{
			return GetColourAttachmentRendererID();
		}

		void Resize( size_t width, size_t height );
		int ReadPixel( uint32_t attachmentIndex, int x, int y ) const;

		void ClearAttachment( uint32_t attachmentIndex, int value );

		void BindColourAttachment( uint32_t index = 0 ) const;
		uint32_t GetColourAttachmentRendererID( uint32_t index = 0 ) const
		{
			ASSERT( index < mColourAttachments.size() );
			return mColourAttachments[ index ];
		}

		const FramebufferSpec& GetSpecification() const
		{
			return mSpecification;
		}

	private:
		void Invalidate();

	private:
		uint32_t mRendererID = 0;
		FramebufferSpec mSpecification;

		std::vector< FramebufferTextureSpec > mColourAttachmentSpecs;
		FramebufferTextureSpec mDepthAttachmentSpec = FramebufferTextureFormat::None;

		std::vector< uint32_t > mColourAttachments;
		uint32_t mDepthAttachment = 0;
	};

} // namespace slc