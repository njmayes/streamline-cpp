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
		static void BeginState(const Matrix4& transform);
		static void EndState();

		static void DrawQuad(const Vector2& position, const Vector2& size, const Vector4& colour);
		static void DrawQuad(const Matrix4& transform, const Vector4& colour);

		static void DrawQuad(const Vector2& position, const Vector2& size, const Ref<IRenderable>& texture, float tilingFactor = 1.0f, const Vector4& tintColour = Vector4(1.0f));
		static void DrawQuad(const Matrix4& transform, const Ref<IRenderable>& textureSlot, float tilingFactor = 1.0f, const Vector4& tintColour = Vector4(1.0f));

		static void DrawRotatedQuad(const Vector2& position, const Vector2& size, float rotation, const Vector4& colour);
		static void DrawRotatedQuad(const Vector2& position, const Vector2& size, float rotation, const Ref<IRenderable>& texture, float tilingFactor = 1.0f, const Vector4& tintColour = Vector4(1.0f));

		static void DrawCircle(const Matrix4& transform, const Vector4& colour, float thickness = 1.0f);

		static void DrawLine(const Vector3& p0, const Vector3& p1, const Vector4& colour);

		static void DrawRect(const Vector2& position, const Vector2& size, const Vector4& colour);
		static void DrawRect(const Matrix4& transform, const Vector4& colour);

		static void ResetStats();
		static const RenderStatistics& GetStats();

	private:
		static void StartBatch();
		static void Flush();
		static void NextBatch();

	private:
		inline static Unique<Renderer2DData> sRenderData;
	};
}