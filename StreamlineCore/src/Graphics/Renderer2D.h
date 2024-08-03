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
		static void BeginState(const glm::mat4& transform);
		static void EndState();

		static void DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& colour, int entityID = -1);
		static void DrawQuad(const glm::mat4& transform, const glm::vec4& colour, int entityID = -1);

		static void DrawQuad(const glm::vec2& position, const glm::vec2& size, const Ref<IRenderable>& texture, float tilingFactor = 1.0f, const glm::vec4& tintColour = glm::vec4(1.0f), int entityID = -1);
		static void DrawQuad(const glm::mat4& transform, const Ref<IRenderable>& textureSlot, float tilingFactor = 1.0f, const glm::vec4& tintColour = glm::vec4(1.0f), int entityID = -1);

		static void DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& colour, int entityID = -1);
		static void DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const Ref<IRenderable>& texture, float tilingFactor = 1.0f, const glm::vec4& tintColour = glm::vec4(1.0f), int entityID = -1);

		static void DrawCircle(const glm::mat4& transform, const glm::vec4& colour, float thickness = 1.0f, int entityID = -1);

		static void DrawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& colour, int entityID = -1);

		static void DrawRect(const glm::vec2& position, const glm::vec2& size, const glm::vec4& colour, int entityID = -1);
		static void DrawRect(const glm::mat4& transform, const glm::vec4& colour, int entityID = -1);

		static void ResetStats();
		static const RenderStatistics& GetStats();

	private:
		static void StartBatch();
		static void Flush();
		static void NextBatch();

	private:
		inline static Impl<Renderer2DData> sRenderData;
	};
}