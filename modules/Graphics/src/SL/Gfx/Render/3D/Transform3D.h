#pragma once

#include <SL/Core/Types/Math.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

namespace sl {

	struct Transform3D
	{
		Vec3f position{ 0.0f };
		glm::quat rotation{ 1.0f, 0.0f, 0.0f, 0.0f };
		Vec3f scale{ 1.0f };

		Mat4f Matrix() const
		{
			return glm::translate( Mat4f( 1.0f ), position ) * glm::toMat4( rotation ) * glm::scale( Mat4f( 1.0f ), scale );
		}
	};

} // namespace sl
