#pragma once

#include <SL/Core/Types/Math.h>
#include <cstdint>

namespace sl {

	class ShadowMap3D
	{
	public:
		explicit ShadowMap3D( uint32_t resolution = 2048 );
		~ShadowMap3D();
		ShadowMap3D( const ShadowMap3D& ) = delete;
		ShadowMap3D& operator=( const ShadowMap3D& ) = delete;
		void Resize( uint32_t resolution );
		void Bind() const;
		uint32_t GetTextureID() const { return mTexture; }
		uint32_t GetResolution() const { return mResolution; }

	private:
		uint32_t mFramebuffer = 0;
		uint32_t mTexture = 0;
		uint32_t mResolution = 0;
	};

} // namespace sl
