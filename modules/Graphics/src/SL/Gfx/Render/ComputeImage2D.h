#pragma once

#include "IRenderable.h"

#include <cstddef>
#include <cstdint>

namespace sl {

	// Strongly typed, GPU-resident image for OpenGL compute (not a CPU pixel buffer).
	class ComputeImage2D : public IRenderable
	{
	public:
		enum class Format { R32Float, RG32Float, R32Unsigned, RGBA8 };
		enum class Access { Read, Write, ReadWrite };

		ComputeImage2D( int width, int height, Format format );
		~ComputeImage2D() override;
		ComputeImage2D( ComputeImage2D const& ) = delete;
		ComputeImage2D& operator=( ComputeImage2D const& ) = delete;

		std::uint32_t GetTextureID() const override { return mTexture; }
		int GetWidth() const { return mWidth; }
		int GetHeight() const { return mHeight; }
		std::size_t GetByteSize() const;

		void BindImage( std::uint32_t unit, Access access = Access::Read ) const;
		void Upload( void const* data, std::size_t size );
		void Download( void* data, std::size_t size ) const;
		void UploadPixel( int x, int y, void const* data );
		void Clear();

	private:
		unsigned int GLFormat() const;
		unsigned int GLDataFormat() const;
		unsigned int GLDataType() const;
		std::size_t GetPixelSize() const;

		int mWidth, mHeight;
		Format mFormat;
		std::uint32_t mTexture = 0;
	};

} // namespace sl
