#pragma once

#include <cstdint>

namespace sl {

	class HDRTarget3D
	{
	public:
		HDRTarget3D( uint32_t width, uint32_t height );
		~HDRTarget3D();
		HDRTarget3D( const HDRTarget3D& ) = delete;
		HDRTarget3D& operator=( const HDRTarget3D& ) = delete;
		void Resize( uint32_t width, uint32_t height );
		void Bind() const;
		uint32_t GetColourTexture() const { return mColour; }
		uint32_t GetWidth() const { return mWidth; }
		uint32_t GetHeight() const { return mHeight; }

	private:
		uint32_t mFramebuffer = 0;
		uint32_t mColour = 0;
		uint32_t mDepthStencil = 0;
		uint32_t mWidth = 0;
		uint32_t mHeight = 0;
	};

} // namespace sl
