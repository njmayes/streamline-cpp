#pragma once

#include <glm/gtc/matrix_transform.hpp>

#include <slc/Common/Base.h>
#include <slc/Types/Math.h>

#include "VertexArray.h"
#include "Shader.h"
#include "Texture.h"
#include "UniformBuffer.h"

namespace slc {

	struct QuadVertex
	{
		Vector3f position;
		Vector4f colour;
		Vector2f texCoord;

		float texture_index;
		float tiling_factor;
	};

	struct CircleVertex
	{
		Vector3f world_position;
		float thickness;
		Vector2f local_position;
		Vector4f colour;
	};

	struct LineVertex
	{
		Vector3f position;
		Vector4f colour;
	};

	struct RenderStatistics
	{
		uint32_t draw_calls = 0;
		uint32_t quad_count = 0;

		uint32_t getTotalVertexCount() const
		{
			return quad_count * 4;
		}
		uint32_t getTotalIndexCount() const
		{
			return quad_count * 6;
		}
	};

	struct Renderer2DData
	{
		static constexpr uint32_t MaxQuads = 20000;
		static constexpr uint32_t MaxVertices = MaxQuads * 4;
		static constexpr uint32_t MaxIndices = MaxQuads * 6;
		static constexpr uint32_t MaxTextureSlots = 32;
		static constexpr Vector4f QuadVertexPositions[ 4 ] = {
			{ -0.5f, -0.5f, 0.0f, 1.0f },
			{ 0.5f, -0.5f, 0.0f, 1.0f },
			{ 0.5f, 0.5f, 0.0f, 1.0f },
			{ -0.5f, 0.5f, 0.0f, 1.0f }
		};


		Ref< VertexArray > quad_vertex_array;
		Ref< VertexBuffer > quad_vertex_buffer;
		Ref< Shader > quad_shader;

		uint32_t quad_index_count = 0;
		QuadVertex* quad_vertex_buffer_base = nullptr;
		QuadVertex* quad_vertex_buffer_ptr = nullptr;


		Ref< VertexArray > circle_vertex_array;
		Ref< VertexBuffer > circle_vertex_buffer;
		Ref< Shader > circle_shader;

		uint32_t circle_index_count = 0;
		CircleVertex* circle_vertex_buffer_base = nullptr;
		CircleVertex* circle_vertex_buffer_ptr = nullptr;


		Ref< VertexArray > line_vertex_array;
		Ref< VertexBuffer > line_vertex_buffer;
		Ref< Shader > line_shader;

		uint32_t line_vertex_count = 0;
		LineVertex* line_vertex_buffer_base = nullptr;
		LineVertex* line_vertex_buffer_ptr = nullptr;

		float line_width = 2.0f;


		Ref< Texture2D > white_texture;
		std::array< Ref< IRenderable >, MaxTextureSlots > texture_slots;
		uint32_t texture_slot_index;


		RenderStatistics stats;

		using CameraData = Matrix4f;
		CameraData camera_matrix;
		Ref< UniformBuffer > camera_uniform_buffer;
	};
} // namespace slc