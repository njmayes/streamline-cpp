#pragma once

#include "IRenderable.h"

#include <Types/Buffer.h>

typedef unsigned int GLenum;

namespace slc {

	class Texture2D : public IRenderable
	{		
	public:
		Texture2D(int width, int height);
		Texture2D(std::string_view path);
		~Texture2D();

		bool Loaded() const { return mRendererID != -1; }

		uint32_t GetTextureID() const override { return mRendererID; }

		int GetWidth() const { return mWidth; }
		int GetHeight() const { return mHeight; }
		uint32_t GetSize() const;

		void SetData(void* data, size_t size);
		void SetData(Buffer buffer);
		Buffer GetData();

	private:
		int mWidth, mHeight;
		uint32_t mRendererID = -1;
		GLenum mInternalFormat, mDataFormat;
	};

}