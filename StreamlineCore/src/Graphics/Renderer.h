#pragma once

#include "Types/Math.h"

#include "VertexArray.h"

namespace slc {

	class Renderer
	{
	public:
		static void Init();
		static void Shutdown();

		static void SetViewport(uint32_t w, uint32_t h);
		static void SetClearColor(const Vector4& colour);
		static void Clear();

		static void SetLineWidth(float width);

		static void DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount = 0);
		static void DrawLines(const Ref<VertexArray>& vertexArray, uint32_t vertexCount);

		static void EnableDepth();
		static void DisableDepth();
	};

}