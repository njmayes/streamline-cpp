#include "Camera3D.h"

#include <glm/gtc/matrix_transform.hpp>

namespace sl {

	void Camera3D::SetPerspective( float verticalFovRadians, float aspectRatio, float nearClip, float farClip )
	{
		mProjection = glm::perspective( verticalFovRadians, aspectRatio, nearClip, farClip );
	}

	void Camera3D::SetOrthographic( float height, float aspectRatio, float nearClip, float farClip )
	{
		mProjection = glm::ortho( -height * aspectRatio * 0.5f, height * aspectRatio * 0.5f,
			-height * 0.5f, height * 0.5f, nearClip, farClip );
	}

	void Camera3D::LookAt( const Vec3f& position, const Vec3f& target, const Vec3f& up )
	{
		mPosition = position;
		mView = glm::lookAt( position, target, up );
	}

	void Camera3D::SetView( const Mat4f& view )
	{
		mView = view;
		mPosition = Vec3f( glm::inverse( mView )[ 3 ] );
	}

	void Camera3D::SetProjection( const Mat4f& projection )
	{
		mProjection = projection;
	}

} // namespace sl
