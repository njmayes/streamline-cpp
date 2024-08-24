#include "PixelBuffer.h"

#include "glad/glad.h"

namespace slc {

#define MAP(x, y) ((y * mWidth) + x)

	PixelBuffer::PixelBuffer(int width, int height)
		: mWidth(width), mHeight(height), mSize(width* height * sizeof(Pixel)), mTexture(Ref<Texture2D>::Create(width, height))
	{
		glGenBuffers(1, &mRendererID);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, mRendererID);
		glBufferData(GL_PIXEL_UNPACK_BUFFER, mSize, nullptr, GL_STREAM_DRAW);
	}

	PixelBuffer::~PixelBuffer()
	{
		glDeleteBuffers(1, &mRendererID);
	}

	void PixelBuffer::Lock()
	{
		ASSERT(!mLocked, "Buffer memory already mapped!");

		mLocked = true;

		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, mRendererID);
		glBufferData(GL_PIXEL_UNPACK_BUFFER, mSize, nullptr, GL_STREAM_DRAW);
		void* data = glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);

		ASSERT(data, "Could not read data from pixel buffer!");
		mPixels = (Pixel*)data;
	}

	void PixelBuffer::Unlock()
	{
		ASSERT(mLocked, "Buffer memory not mapped!");

		mLocked = false;

		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, mRendererID);
		int success = glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
		ASSERT(success, "Could not unmap buffer");

		glTextureSubImage2D(mTexture->GetTextureID(), 0, 0, 0, mWidth, mHeight, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	}

	Pixel& PixelBuffer::At(size_t x, size_t y)
	{
		ASSERT(mLocked, "Buffer must be locked!");
		return mPixels[MAP(x, y)];
	}

	const Pixel& PixelBuffer::At(size_t x, size_t y) const
	{
		ASSERT(mLocked, "Buffer must be locked!");
		return mPixels[MAP(x, y)];
	}

	void PixelBuffer::Set(const Pixel& colour)
	{
		ASSERT(mLocked, "Buffer must be locked!");
		std::fill_n(mPixels, mWidth * mHeight, colour);
	}

	void PixelBuffer::Set(const Pixel& colour, size_t size, size_t offset)
	{
		ASSERT(mLocked, "Buffer must be locked!");
		std::fill_n(mPixels + offset, size, colour);
	}
}