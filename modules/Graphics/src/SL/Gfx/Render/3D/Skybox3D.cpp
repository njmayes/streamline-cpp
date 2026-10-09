#include "Skybox3D.h"

#include <glad/gl.h>
#include <stb_image.h>
#include <stdexcept>
#include <cmath>

namespace sl {

	Skybox3D::Skybox3D( const std::array< std::filesystem::path, 6 >& faces )
	{
		std::array< unsigned char*, 6 > pixels{};
		int width = 0, height = 0;
		stbi_set_flip_vertically_on_load( 0 );
		try
		{
			for ( size_t i = 0; i < 6; ++i )
			{
				int faceWidth = 0, faceHeight = 0, components = 0;
				const std::string path = faces[ i ].string();
				pixels[ i ] = stbi_load( path.c_str(), &faceWidth, &faceHeight, &components, 4 );
				if ( !pixels[ i ] || faceWidth != faceHeight || ( width && faceWidth != width ) )
					throw std::runtime_error( "Skybox faces must be square images with identical dimensions: " + path );
				width = faceWidth; height = faceHeight;
			}
			glCreateTextures( GL_TEXTURE_CUBE_MAP, 1, &mTexture );
			const int levels = 1 + static_cast< int >( std::floor( std::log2( width ) ) );
			glTextureStorage2D( mTexture, levels, GL_SRGB8_ALPHA8, width, height );
			for ( uint32_t i = 0; i < 6; ++i )
				glTextureSubImage3D( mTexture, 0, 0, 0, i, width, height, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixels[ i ] );
			glGenerateTextureMipmap( mTexture );
			glTextureParameteri( mTexture, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
			glTextureParameteri( mTexture, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
			glTextureParameteri( mTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
			glTextureParameteri( mTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
			glTextureParameteri( mTexture, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE );
		}
		catch ( ... )
		{
			for ( auto* face : pixels ) stbi_image_free( face );
			glDeleteTextures( 1, &mTexture );
			throw;
		}
		for ( auto* face : pixels ) stbi_image_free( face );
	}

	Skybox3D::~Skybox3D()
	{
		glDeleteTextures( 1, &mTexture );
	}

} // namespace sl
