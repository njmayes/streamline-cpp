#include "Renderer2D.h"

#include "Renderer.h"

namespace slc {

	void Renderer2D::Init()
	{
		sRenderData = MakeUnique<Renderer2DData>();

		{	// Quads
			sRenderData->quadVertexArray = Ref<VertexArray>::Create();

			sRenderData->quadVertexBuffer = Ref<VertexBuffer>::Create(Renderer2DData::MaxVertices * (uint32_t)sizeof(QuadVertex));
			sRenderData->quadVertexBuffer->SetLayout({
				{ ShaderDataType::Float3, "aPosition"	  },
				{ ShaderDataType::Float4, "aColour"		  },
				{ ShaderDataType::Float2, "aTexCoord"	  },
				{ ShaderDataType::Float,  "aTexIndex"	  },
				{ ShaderDataType::Float,  "aTilingFactor" }
				});

			sRenderData->quadVertexArray->AddVertexBuffer(sRenderData->quadVertexBuffer);

			sRenderData->quadVertexBufferBase = new QuadVertex[Renderer2DData::MaxVertices];

			uint32_t* quadIndices = new uint32_t[Renderer2DData::MaxIndices];

			uint32_t offset = 0;
			for (uint32_t i = 0; i < Renderer2DData::MaxIndices; i += 6)
			{
				quadIndices[i + 0] = offset + 0;
				quadIndices[i + 1] = offset + 1;
				quadIndices[i + 2] = offset + 2;

				quadIndices[i + 3] = offset + 2;
				quadIndices[i + 4] = offset + 3;
				quadIndices[i + 5] = offset + 0;

				offset += 4;
			}

			Ref<IndexBuffer> quadIB = Ref<IndexBuffer>::Create(quadIndices, Renderer2DData::MaxIndices);
			sRenderData->quadVertexArray->SetIndexBuffer(quadIB);
			delete[] quadIndices;

			sRenderData->quadShader = Ref<Shader>::Create("resources/shaders/Renderer2DQuad.glsl");
		}

		// Circles
		{
			sRenderData->circleVertexArray = Ref<VertexArray>::Create();

			sRenderData->circleVertexBuffer = Ref<VertexBuffer>::Create(sRenderData->MaxVertices * (uint32_t)sizeof(QuadVertex));
			sRenderData->circleVertexBuffer->SetLayout({
				{ ShaderDataType::Float3, "aWorldPosition" },
				{ ShaderDataType::Float,  "aThickness"     },
				{ ShaderDataType::Float2, "aLocalPosition" },
				{ ShaderDataType::Float4, "aColour"        }
				});

			sRenderData->circleVertexArray->AddVertexBuffer(sRenderData->circleVertexBuffer);

			sRenderData->circleVertexBufferBase = new CircleVertex[sRenderData->MaxVertices];
			sRenderData->circleVertexArray->SetIndexBuffer(sRenderData->quadVertexArray->GetIndexBuffer()); // Reuse quad index buffer

			sRenderData->circleShader = Ref<Shader>::Create("resources/shaders/Renderer2DCircle.glsl");
		}

		// Lines
		{
			sRenderData->lineVertexArray = Ref<VertexArray>::Create();

			sRenderData->lineVertexBuffer = Ref<VertexBuffer>::Create(sRenderData->MaxVertices * (uint32_t)sizeof(QuadVertex));
			sRenderData->lineVertexBuffer->SetLayout({
				{ ShaderDataType::Float3, "aPosition" },
				{ ShaderDataType::Float4, "aColour"   }
				});

			sRenderData->lineVertexArray->AddVertexBuffer(sRenderData->lineVertexBuffer);
			sRenderData->lineVertexBufferBase = new LineVertex[sRenderData->MaxVertices];

			sRenderData->lineShader = Ref<Shader>::Create("resources/shaders/Renderer2DLine.glsl");
		}

		// White Texture
		sRenderData->whiteTexture = Ref<Texture2D>::Create(1, 1);
		uint32_t whiteTextureData = 0xffffffff;
		sRenderData->whiteTexture->SetData(&whiteTextureData, sizeof(uint32_t));

		sRenderData->textureSlots[0] = sRenderData->whiteTexture;

		sRenderData->cameraUniformBuffer = Ref<UniformBuffer>::Create((uint32_t)sizeof(Renderer2DData::CameraData), 0);
	}

	void Renderer2D::Shutdown()
	{
		delete[] sRenderData->quadVertexBufferBase;
		delete[] sRenderData->circleVertexBufferBase;
		delete[] sRenderData->lineVertexBufferBase;

		sRenderData.reset();
	}

	void Renderer2D::BeginState()
	{
		BeginState(Matrix4{ 1.0f });
	}

	void Renderer2D::BeginState(const Matrix4& cameraTransform)
	{
		sRenderData->cameraMatrix = cameraTransform;
		sRenderData->cameraUniformBuffer->SetData(&sRenderData->cameraMatrix, sizeof(Renderer2DData::CameraData));

		StartBatch();
	}

	void Renderer2D::EndState()
	{
		Flush();
	}

	void Renderer2D::DrawQuad(const Vector2& position, const Vector2& size, const Vector4& colour)
	{
		Matrix4 transform = glm::translate(Matrix4(1.0f), { position, 0.0f })
			* glm::scale(Matrix4(1.0f), { size.x, size.y, 1.0f });

		DrawQuad(transform, colour);
	}

	void Renderer2D::DrawQuad(const Vector2& position, const Vector2& size, const Ref<IRenderable>& texture, float tilingFactor, const Vector4& tintColour)
	{
		Matrix4 transform = glm::translate(Matrix4(1.0f), { position, 0.0f })
			* glm::scale(Matrix4(1.0f), { size.x, size.y, 1.0f });

		DrawQuad(transform, texture, tilingFactor, tintColour);
	}

	void Renderer2D::DrawRotatedQuad(const Vector2& position, const Vector2& size, float rotation, const Vector4& colour)
	{
		Matrix4 transform = glm::translate(Matrix4(1.0f), { position, 0.0f })
			* glm::rotate(Matrix4(1.0f), rotation, { 0.0f, 0.0f, 1.0f })
			* glm::scale(Matrix4(1.0f), { size.x, size.y, 1.0f });

		DrawQuad(transform, colour);
	}

	void Renderer2D::DrawRotatedQuad(const Vector2& position, const Vector2& size, float rotation, const Ref<IRenderable>& texture, float tilingFactor, const Vector4& tintColour)
	{
		Matrix4 transform = glm::translate(Matrix4(1.0f), { position, 0.0f })
			* glm::rotate(Matrix4(1.0f), rotation, { 0.0f, 0.0f, 1.0f })
			* glm::scale(Matrix4(1.0f), { size.x, size.y, 1.0f });

		DrawQuad(transform, texture, tilingFactor, tintColour);
	}

	void Renderer2D::DrawQuad(const Matrix4& transform, const Vector4& colour)
	{
		constexpr size_t quadVertexCount = 4;
		constexpr Vector2 textureCoords[4] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };
		constexpr float textureIndex = 0.0f; // White Texture
		constexpr float tilingFactor = 1.0f;

		if (sRenderData->quadIndexCount >= Renderer2DData::MaxIndices)
			NextBatch();

		for (size_t i = 0; i < quadVertexCount; i++)
		{
			sRenderData->quadVertexBufferPtr->position = transform * Renderer2DData::QuadVertexPositions[i];
			sRenderData->quadVertexBufferPtr->colour = colour;
			sRenderData->quadVertexBufferPtr->texCoord = textureCoords[i];
			sRenderData->quadVertexBufferPtr->texIndex = textureIndex;
			sRenderData->quadVertexBufferPtr->tilingFactor = tilingFactor;
			sRenderData->quadVertexBufferPtr++;
		}

		sRenderData->quadIndexCount += 6;
		sRenderData->stats.quadCount++;
	}

	void Renderer2D::DrawQuad(const Matrix4& transform, const Ref<IRenderable>& textureSlot, float tilingFactor, const Vector4& tintColour)
	{
		constexpr size_t quadVertexCount = 4;
		const Vector2* textureCoords = textureSlot->GetTextureCoords();

		if (sRenderData->quadIndexCount >= Renderer2DData::MaxIndices)
			NextBatch();

		float textureIndex = 0.0f;
		for (uint32_t i = 1; i < sRenderData->textureSlotIndex; i++)
		{
			if (*sRenderData->textureSlots[i] == *textureSlot)
			{
				textureIndex = (float)i;
				break;
			}
		}

		if (textureIndex == 0.0f)
		{
			if (sRenderData->textureSlotIndex >= Renderer2DData::MaxTextureSlots)
				NextBatch();

			textureIndex = (float)sRenderData->textureSlotIndex;
			sRenderData->textureSlots[sRenderData->textureSlotIndex] = textureSlot;
			sRenderData->textureSlotIndex++;
		}

		for (size_t i = 0; i < quadVertexCount; i++)
		{
			sRenderData->quadVertexBufferPtr->position = transform * Renderer2DData::QuadVertexPositions[i];
			sRenderData->quadVertexBufferPtr->colour = tintColour;
			sRenderData->quadVertexBufferPtr->texCoord = textureCoords[i];
			sRenderData->quadVertexBufferPtr->texIndex = textureIndex;
			sRenderData->quadVertexBufferPtr->tilingFactor = tilingFactor;
			sRenderData->quadVertexBufferPtr++;
		}

		sRenderData->quadIndexCount += 6;
		sRenderData->stats.quadCount++;
	}

	void Renderer2D::DrawLine(const Vector3& p0, const Vector3& p1, const Vector4& colour)
	{
		sRenderData->lineVertexBufferPtr->position = p0;
		sRenderData->lineVertexBufferPtr->colour = colour;
		sRenderData->lineVertexBufferPtr++;

		sRenderData->lineVertexBufferPtr->position = p1;
		sRenderData->lineVertexBufferPtr->colour = colour;
		sRenderData->lineVertexBufferPtr++;

		sRenderData->lineVertexCount += 2;
	}

	void Renderer2D::DrawRect(const Vector2& position, const Vector2& size, const Vector4& colour)
	{
		Matrix4 transform = glm::translate(Matrix4(1.0f), { position, 0.0f })
			* glm::scale(Matrix4(1.0f), { size.x, size.y, 1.0f });

		DrawRect(transform, colour);
	}

	void Renderer2D::DrawRect(const Matrix4& transform, const Vector4& colour)
	{
		Vector3 lineVertices[4];
		for (size_t i = 0; i < 4; i++)
			lineVertices[i] = transform * Renderer2DData::QuadVertexPositions[i];

		DrawLine(lineVertices[0], lineVertices[1], colour);
		DrawLine(lineVertices[1], lineVertices[2], colour);
		DrawLine(lineVertices[2], lineVertices[3], colour);
		DrawLine(lineVertices[3], lineVertices[0], colour);
	}

	void Renderer2D::DrawCircle(const Matrix4& transform, const Vector4& colour, float thickness)
	{
		if (sRenderData->circleIndexCount >= Renderer2DData::MaxIndices)
			NextBatch();

		for (size_t i = 0; i < 4; i++)
		{
			sRenderData->circleVertexBufferPtr->worldPosition = transform * Renderer2DData::QuadVertexPositions[i];
			sRenderData->circleVertexBufferPtr->thickness = thickness;
			sRenderData->circleVertexBufferPtr->localPosition = Renderer2DData::QuadVertexPositions[i] * 2.0f;
			sRenderData->circleVertexBufferPtr->colour = colour;
			sRenderData->circleVertexBufferPtr++;
		}

		sRenderData->circleIndexCount += 6;

		sRenderData->stats.quadCount++;
	}

	void Renderer2D::StartBatch()
	{
		sRenderData->quadIndexCount = 0;
		sRenderData->quadVertexBufferPtr = sRenderData->quadVertexBufferBase;

		sRenderData->circleIndexCount = 0;
		sRenderData->circleVertexBufferPtr = sRenderData->circleVertexBufferBase;

		sRenderData->lineVertexCount = 0;
		sRenderData->lineVertexBufferPtr = sRenderData->lineVertexBufferBase;

		sRenderData->textureSlotIndex = 1;
	}

	void Renderer2D::Flush()
	{
		// Quads
		if (sRenderData->quadIndexCount)
		{
			uint32_t quadDataSize = (uint32_t)((uint8_t*)sRenderData->quadVertexBufferPtr - (uint8_t*)sRenderData->quadVertexBufferBase);
			sRenderData->quadVertexBuffer->SetData(sRenderData->quadVertexBufferBase, quadDataSize);

			for (uint32_t i = 0; i < sRenderData->textureSlotIndex; i++)
				sRenderData->textureSlots[i]->BindTexture(i);

			sRenderData->quadShader->Bind();
			Renderer::DrawIndexed(sRenderData->quadVertexArray, sRenderData->quadIndexCount);
			sRenderData->stats.drawCalls++;
		}

		// Circles
		if (sRenderData->circleIndexCount)
		{
			uint32_t circleDataSize = (uint32_t)((uint8_t*)sRenderData->circleVertexBufferPtr - (uint8_t*)sRenderData->circleVertexBufferBase);
			sRenderData->circleVertexBuffer->SetData(sRenderData->circleVertexBufferBase, circleDataSize);

			sRenderData->circleShader->Bind();
			Renderer::DrawIndexed(sRenderData->circleVertexArray, sRenderData->circleIndexCount);
			sRenderData->stats.drawCalls++;
		}

		// Lines
		if (sRenderData->lineVertexCount)
		{
			uint32_t lineDataSize = (uint32_t)((uint8_t*)sRenderData->lineVertexBufferPtr - (uint8_t*)sRenderData->lineVertexBufferBase);
			sRenderData->lineVertexBuffer->SetData(sRenderData->lineVertexBufferBase, lineDataSize);

			sRenderData->lineShader->Bind();
			Renderer::SetLineWidth(sRenderData->lineWidth);
			Renderer::DrawLines(sRenderData->lineVertexArray, sRenderData->lineVertexCount);
			sRenderData->stats.drawCalls++;
		}
	}

	void Renderer2D::NextBatch()
	{
		Flush();
		StartBatch();
	}

	void Renderer2D::ResetStats()
	{
		memset(&sRenderData->stats, 0, sizeof(RenderStatistics));
	}

	const RenderStatistics& Renderer2D::GetStats()
	{
		return sRenderData->stats;
	}
}
