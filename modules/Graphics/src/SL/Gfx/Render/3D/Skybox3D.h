#pragma once

#include <SL/Core/Common/Base.h>
#include <array>
#include <filesystem>
#include <cstdint>

namespace sl {

	class Skybox3D : public RefCounted
	{
	public:
		// GL convention: +X, -X, +Y, -Y, +Z, -Z.
		explicit Skybox3D( const std::array< std::filesystem::path, 6 >& faces );
		~Skybox3D();
		Skybox3D( const Skybox3D& ) = delete;
		Skybox3D& operator=( const Skybox3D& ) = delete;
		uint32_t GetTextureID() const { return mTexture; }

	private:
		uint32_t mTexture = 0;
	};

} // namespace sl
