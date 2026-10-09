#pragma once

#include "SL/Gfx/Render/Texture.h"

namespace sl {

	enum class AlphaMode3D : uint8_t { Opaque, Mask, Blend };

	struct Material3D
	{
		Vec4f baseColour{ 1.0f };
		Vec3f emissive{ 0.0f };
		float metallic = 0.0f;
		float roughness = 0.8f;
		float normalScale = 1.0f;
		float alphaCutoff = 0.5f;
		AlphaMode3D alphaMode = AlphaMode3D::Opaque;
		bool doubleSided = false;
		Ref< Texture2D > baseColourTexture;
		Ref< Texture2D > normalTexture;
		Ref< Texture2D > metallicRoughnessTexture; // G=roughness, B=metallic (glTF convention).
		Ref< Texture2D > emissiveTexture;
	};

} // namespace sl
