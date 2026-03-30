#pragma once

#include "IRenderable.h"

#include <SL/Core/Types/Buffer.h>

typedef unsigned int GLenum;

namespace sl {

	class Texture2D : public IRenderable
	{
	public:
		Texture2D( int width, int height );
		Texture2D( std::string_view path );
		~Texture2D();

		Texture2D( Texture2D const& ) = delete;
		Texture2D& operator=( Texture2D const& ) = delete;

		Texture2D( Texture2D&& other ) noexcept;
		Texture2D& operator=( Texture2D&& other ) noexcept;

		bool Loaded() const
		{
			return mRendererID != 0;
		}

		uint32_t GetTextureID() const override
		{
			return mRendererID;
		}

		int GetWidth() const
		{
			return mWidth;
		}
		int GetHeight() const
		{
			return mHeight;
		}
		uint32_t GetSize() const;

		void SetData( void* data, size_t size );
		void SetData( Buffer buffer );
		Buffer GetData();

	private:
		int mWidth, mHeight;
		uint32_t mRendererID = 0;
		GLenum mInternalFormat, mDataFormat;
	};

} // namespace sl