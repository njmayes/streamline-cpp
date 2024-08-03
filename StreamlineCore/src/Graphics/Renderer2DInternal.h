#pragma once

#include <glm/gtc/matrix_transform.hpp>

#include <Common/Base.h>
#include <Types/Math.h>

#include "VertexArray.h"
#include "Shader.h"
#include "Texture.h"
#include "UniformBuffer.h"

namespace slc {

	struct QuadVertex
	{
		Vector3 position;
		Vector4 colour;
		Vector2 texCoord;

		float texIndex;
		float tilingFactor;
	};

	struct CircleVertex
	{
		Vector3 worldPosition;
		float thickness;
		Vector2 localPosition;
		Vector4 colour;
	};

	struct LineVertex
	{
		Vector3 position;
		Vector4 colour;
	};

	struct RenderStatistics
	{
		uint32_t drawCalls = 0;
		uint32_t quadCount = 0;

		uint32_t getTotalVertexCount() const { return quadCount * 4; }
		uint32_t getTotalIndexCount() const { return quadCount * 6; }
	};

	struct Renderer2DData
	{
		static constexpr uint32_t MaxQuads = 20000;
		static constexpr uint32_t MaxVertices = MaxQuads * 4;
		static constexpr uint32_t MaxIndices = MaxQuads * 6;
		static constexpr uint32_t MaxTextureSlots = 32;
		static constexpr Vector4 QuadVertexPositions[4] = 
		{ 
			{ -0.5f, -0.5f, 0.0f, 1.0f },
			{  0.5f, -0.5f, 0.0f, 1.0f },
			{  0.5f,  0.5f, 0.0f, 1.0f },
			{ -0.5f,  0.5f, 0.0f, 1.0f } 
		};


		Ref<VertexArray> quadVertexArray;
		Ref<VertexBuffer> quadVertexBuffer;
		Ref<Shader> quadShader;

		uint32_t quadIndexCount = 0;
		QuadVertex* quadVertexBufferBase = nullptr;
		QuadVertex* quadVertexBufferPtr = nullptr;


		Ref<VertexArray> circleVertexArray;
		Ref<VertexBuffer> circleVertexBuffer;
		Ref<Shader> circleShader;

		uint32_t circleIndexCount = 0;
		CircleVertex* circleVertexBufferBase = nullptr;
		CircleVertex* circleVertexBufferPtr = nullptr;


		Ref<VertexArray> lineVertexArray;
		Ref<VertexBuffer> lineVertexBuffer;
		Ref<Shader> lineShader;

		uint32_t lineVertexCount = 0;
		LineVertex* lineVertexBufferBase = nullptr;
		LineVertex* lineVertexBufferPtr = nullptr;

		float lineWidth = 2.0f;


		Ref<Texture2D> whiteTexture;
		std::array<Ref<IRenderable>, MaxTextureSlots> textureSlots;
		uint32_t textureSlotIndex;


		RenderStatistics stats;

		using CameraData = Matrix4;
		CameraData cameraMatrix;
		Ref<UniformBuffer> cameraUniformBuffer;
	};
}