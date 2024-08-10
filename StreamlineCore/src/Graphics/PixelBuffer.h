#pragma once

#include "Texture.h"
#include "Pixel.h"

namespace slc {

	class PixelBuffer : public IRenderable
	{
	public:
		PixelBuffer(int width, int height);
		~PixelBuffer();

		void Lock();
		void Unlock();

		uint32_t GetTextureID() const override { return mTexture->GetTextureID(); }

		Pixel& At(size_t x, size_t y);
		const Pixel& At(size_t x, size_t y) const;

		Pixel* Get(size_t offset = 0) { return mPixels + offset; }
		const Pixel* Get(size_t offset = 0) const { return mPixels + offset; }
		void Set(const Pixel& colour);
		void Set(const Pixel& colour, size_t size, size_t offset);

	private:
		uint32_t mRendererID = 0;
		int mSize = 0;
		int mWidth = 0, mHeight = 0;

		bool mLocked = false;

		Ref<Texture2D> mTexture = nullptr;
		Pixel* mPixels = nullptr;
	};
}