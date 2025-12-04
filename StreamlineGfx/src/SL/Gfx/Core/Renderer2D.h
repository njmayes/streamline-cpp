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
		static void BeginState( const Mat4f& transform );
		static void EndState();

		static void DrawQuad( const Vec2f& position, const Vec2f& size, const Vec4f& colour );
		static void DrawQuad( const Mat4f& transform, const Vec4f& colour );

		static void DrawQuad( const Vec2f& position, const Vec2f& size, const Ref< IRenderable >& texture, float tiling_factor = 1.0f, const Vec4f& tint_colour = Vec4f( 1.0f ) );
		static void DrawQuad( const Mat4f& transform, const Ref< IRenderable >& textureSlot, float tiling_factor = 1.0f, const Vec4f& tint_colour = Vec4f( 1.0f ) );

		static void DrawRotatedQuad( const Vec2f& position, const Vec2f& size, float rotation, const Vec4f& colour );
		static void DrawRotatedQuad( const Vec2f& position, const Vec2f& size, float rotation, const Ref< IRenderable >& texture, float tiling_factor = 1.0f, const Vec4f& tint_colour = Vec4f( 1.0f ) );

		static void DrawCircle( const Mat4f& transform, const Vec4f& colour, float thickness = 1.0f );

		static void DrawLine( const Vec3f& p0, const Vec3f& p1, const Vec4f& colour );

		static void DrawRect( const Vec2f& position, const Vec2f& size, const Vec4f& colour );
		static void DrawRect( const Mat4f& transform, const Vec4f& colour );

		static void ResetStats();
		static const RenderStatistics& GetStats();

	private:
		static void StartBatch();
		static void Flush();
		static void NextBatch();

	private:
		inline static Box< Renderer2DData > sRenderData;
	};
} // namespace slc