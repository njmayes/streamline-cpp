#pragma once

#include "IRenderable.h"
#include "Renderer2DInternal.h"

namespace slc {

	class Camera;

	class Renderer2D
	{
	public:
		static void Init();
		static void Shutdown();

		static void BeginState();
		static void BeginState( const Matrix4f& transform );
		static void EndState();

		static void DrawQuad( const Vector2f& position, const Vector2f& size, const Vector4f& colour );
		static void DrawQuad( const Matrix4f& transform, const Vector4f& colour );

		static void DrawQuad( const Vector2f& position, const Vector2f& size, const Ref< IRenderable >& texture, float tilingFactor = 1.0f, const Vector4f& tintColour = Vector4f( 1.0f ) );
		static void DrawQuad( const Matrix4f& transform, const Ref< IRenderable >& textureSlot, float tilingFactor = 1.0f, const Vector4f& tintColour = Vector4f( 1.0f ) );

		static void DrawRotatedQuad( const Vector2f& position, const Vector2f& size, float rotation, const Vector4f& colour );
		static void DrawRotatedQuad( const Vector2f& position, const Vector2f& size, float rotation, const Ref< IRenderable >& texture, float tilingFactor = 1.0f, const Vector4f& tintColour = Vector4f( 1.0f ) );

		static void DrawCircle( const Matrix4f& transform, const Vector4f& colour, float thickness = 1.0f );

		static void DrawLine( const Vector3f& p0, const Vector3f& p1, const Vector4f& colour );

		static void DrawRect( const Vector2f& position, const Vector2f& size, const Vector4f& colour );
		static void DrawRect( const Matrix4f& transform, const Vector4f& colour );

		static void ResetStats();
		static const RenderStatistics& GetStats();

	private:
		static void StartBatch();
		static void Flush();
		static void NextBatch();

	private:
		inline static Unique< Renderer2DData > sRenderData;
	};
} // namespace slc