#pragma once

#include <SL/Core/Types/Math.h>
#include <cstdint>

namespace sl {

	struct DirectionalLight3D
	{
		// Direction FROM the light toward the scene (world space).
		Vec3f direction{ -0.4f, -1.0f, -0.4f };
		Vec3f colour{ 1.0f };
		float intensity = 3.0f;
		bool castShadows = true;
	};

	struct PointLight3D
	{
		Vec3f position{ 0.0f };
		Vec3f colour{ 1.0f };
		float intensity = 50.0f;
		float range = 10.0f;
	};

	struct SpotLight3D
	{
		Vec3f position{ 0.0f };
		Vec3f direction{ 0.0f, -1.0f, 0.0f };
		Vec3f colour{ 1.0f };
		float intensity = 50.0f;
		float range = 15.0f;
		float innerAngleRadians = 0.25f;
		float outerAngleRadians = 0.4f;
	};

} // namespace sl
