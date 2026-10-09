#pragma once

#include <SL/Core/Types/Math.h>

namespace sl {

	class Camera3D
	{
	public:
		void SetPerspective( float verticalFovRadians, float aspectRatio, float nearClip = 0.1f, float farClip = 1000.0f );
		void SetOrthographic( float height, float aspectRatio, float nearClip = 0.1f, float farClip = 1000.0f );
		void LookAt( const Vec3f& position, const Vec3f& target, const Vec3f& up = { 0.0f, 1.0f, 0.0f } );
		void SetView( const Mat4f& view );
		void SetProjection( const Mat4f& projection );

		const Mat4f& GetView() const { return mView; }
		const Mat4f& GetProjection() const { return mProjection; }
		Mat4f GetViewProjection() const { return mProjection * mView; }
		const Vec3f& GetPosition() const { return mPosition; }

	private:
		Mat4f mView{ 1.0f };
		Mat4f mProjection{ 1.0f };
		Vec3f mPosition{ 0.0f };
	};

} // namespace sl
