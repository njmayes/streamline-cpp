#include "Renderer2D.h"

#include "Renderer.h"

namespace sl {

	void Renderer2D::Init()
	{
		sRenderData = MakeBox< Renderer2DData >();

		{ // Quads
			sRenderData->quad_vertex_array = Ref< VertexArray >::Create();

			sRenderData->quad_vertex_buffer = Ref< VertexBuffer >::Create( Renderer2DData::MaxVertices * ( uint32_t )sizeof( QuadVertex ) );
			sRenderData->quad_vertex_buffer->SetLayout( { { ShaderDataType::Float3, "aPosition" },
														{ ShaderDataType::Float4, "aColour" },
														{ ShaderDataType::Float2, "aTexCoord" },
														{ ShaderDataType::Float, "aTexIndex" },
														{ ShaderDataType::Float, "aTilingFactor" } } );

			sRenderData->quad_vertex_array->AddVertexBuffer( sRenderData->quad_vertex_buffer );

			sRenderData->quad_vertex_buffer_base = new QuadVertex[ Renderer2DData::MaxVertices ];

			uint32_t* quadIndices = new uint32_t[ Renderer2DData::MaxIndices ];

			uint32_t offset = 0;
			for ( uint32_t i = 0; i < Renderer2DData::MaxIndices; i += 6 )
			{
				quadIndices[ i + 0 ] = offset + 0;
				quadIndices[ i + 1 ] = offset + 1;
				quadIndices[ i + 2 ] = offset + 2;

				quadIndices[ i + 3 ] = offset + 2;
				quadIndices[ i + 4 ] = offset + 3;
				quadIndices[ i + 5 ] = offset + 0;

				offset += 4;
			}

			Ref< IndexBuffer > quadIB = Ref< IndexBuffer >::Create( quadIndices, Renderer2DData::MaxIndices );
			sRenderData->quad_vertex_array->SetIndexBuffer( quadIB );
			delete[] quadIndices;

			sRenderData->quad_shader = Ref< Shader >::Create( "resources/shaders/Renderer2DQuad.glsl" );
		}

		// Circles
		{
			sRenderData->circle_vertex_array = Ref< VertexArray >::Create();

			sRenderData->circle_vertex_buffer = Ref< VertexBuffer >::Create( sRenderData->MaxVertices * ( uint32_t )sizeof( QuadVertex ) );
			sRenderData->circle_vertex_buffer->SetLayout( { { ShaderDataType::Float3, "aWorldPosition" },
														  { ShaderDataType::Float, "aThickness" },
														  { ShaderDataType::Float2, "aLocalPosition" },
														  { ShaderDataType::Float4, "aColour" } } );

			sRenderData->circle_vertex_array->AddVertexBuffer( sRenderData->circle_vertex_buffer );

			sRenderData->circle_vertex_buffer_base = new CircleVertex[ sRenderData->MaxVertices ];
			sRenderData->circle_vertex_array->SetIndexBuffer( sRenderData->quad_vertex_array->GetIndexBuffer() ); // Reuse quad index buffer

			sRenderData->circle_shader = Ref< Shader >::Create( "resources/shaders/Renderer2DCircle.glsl" );
		}

		// Lines
		{
			sRenderData->line_vertex_array = Ref< VertexArray >::Create();

			sRenderData->line_vertex_buffer = Ref< VertexBuffer >::Create( sRenderData->MaxVertices * ( uint32_t )sizeof( QuadVertex ) );
			sRenderData->line_vertex_buffer->SetLayout( { { ShaderDataType::Float3, "aPosition" },
														{ ShaderDataType::Float4, "aColour" } } );

			sRenderData->line_vertex_array->AddVertexBuffer( sRenderData->line_vertex_buffer );
			sRenderData->line_vertex_buffer_base = new LineVertex[ sRenderData->MaxVertices ];

			sRenderData->line_shader = Ref< Shader >::Create( "resources/shaders/Renderer2DLine.glsl" );
		}

		// White Texture
		sRenderData->white_texture = Ref< Texture2D >::Create( 1, 1 );
		uint32_t whiteTextureData = 0xffffffff;
		sRenderData->white_texture->SetData( &whiteTextureData, sizeof( uint32_t ) );

		sRenderData->texture_slots[ 0 ] = sRenderData->white_texture;

		sRenderData->camera_uniform_buffer = Ref< UniformBuffer >::Create( ( uint32_t )sizeof( Renderer2DData::CameraData ), 0 );
	}

	void Renderer2D::Shutdown()
	{
		delete[] sRenderData->quad_vertex_buffer_base;
		delete[] sRenderData->circle_vertex_buffer_base;
		delete[] sRenderData->line_vertex_buffer_base;

		sRenderData.reset();
	}

	void Renderer2D::BeginState()
	{
		BeginState( Mat4f{ 1.0f } );
	}

	void Renderer2D::BeginState( const Mat4f& cameraTransform )
	{
		sRenderData->camera_matrix = cameraTransform;
		sRenderData->camera_uniform_buffer->SetData( &sRenderData->camera_matrix, sizeof( Renderer2DData::CameraData ) );

		StartBatch();
	}

	void Renderer2D::EndState()
	{
		Flush();
	}

	void Renderer2D::DrawQuad( const Vec2f& position, const Vec2f& size, const Vec4f& colour )
	{
		Mat4f transform = glm::translate( Mat4f( 1.0f ), { position, 0.0f } ) * glm::scale( Mat4f( 1.0f ), { size.x, size.y, 1.0f } );

		DrawQuad( transform, colour );
	}

	void Renderer2D::DrawQuad( const Vec2f& position, const Vec2f& size, const Ref< IRenderable >& texture, float tiling_factor, const Vec4f& tint_colour )
	{
		Mat4f transform = glm::translate( Mat4f( 1.0f ), { position, 0.0f } ) * glm::scale( Mat4f( 1.0f ), { size.x, size.y, 1.0f } );

		DrawQuad( transform, texture, tiling_factor, tint_colour );
	}

	void Renderer2D::DrawRotatedQuad( const Vec2f& position, const Vec2f& size, float rotation, const Vec4f& colour )
	{
		Mat4f transform = glm::translate( Mat4f( 1.0f ), { position, 0.0f } ) * glm::rotate( Mat4f( 1.0f ), rotation, { 0.0f, 0.0f, 1.0f } ) * glm::scale( Mat4f( 1.0f ), { size.x, size.y, 1.0f } );

		DrawQuad( transform, colour );
	}

	void Renderer2D::DrawRotatedQuad( const Vec2f& position, const Vec2f& size, float rotation, const Ref< IRenderable >& texture, float tiling_factor, const Vec4f& tint_colour )
	{
		Mat4f transform = glm::translate( Mat4f( 1.0f ), { position, 0.0f } ) * glm::rotate( Mat4f( 1.0f ), rotation, { 0.0f, 0.0f, 1.0f } ) * glm::scale( Mat4f( 1.0f ), { size.x, size.y, 1.0f } );

		DrawQuad( transform, texture, tiling_factor, tint_colour );
	}

	void Renderer2D::DrawQuad( const Mat4f& transform, const Vec4f& colour )
	{
		constexpr size_t quad_vertex_count = 4;
		constexpr Vec2f texture_coords[ 4 ] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };
		constexpr float texture_index = 0.0f; // White Texture
		constexpr float tiling_factor = 1.0f;

		if ( sRenderData->quad_index_count >= Renderer2DData::MaxIndices )
			NextBatch();

		for ( size_t i = 0; i < quad_vertex_count; i++ )
		{
			sRenderData->quad_vertex_buffer_ptr->position = transform * Renderer2DData::QuadVertexPositions[ i ];
			sRenderData->quad_vertex_buffer_ptr->colour = colour;
			sRenderData->quad_vertex_buffer_ptr->texCoord = texture_coords[ i ];
			sRenderData->quad_vertex_buffer_ptr->texture_index = texture_index;
			sRenderData->quad_vertex_buffer_ptr->tiling_factor = tiling_factor;
			sRenderData->quad_vertex_buffer_ptr++;
		}

		sRenderData->quad_index_count += 6;
		sRenderData->stats.quad_count++;
	}

	void Renderer2D::DrawQuad( const Mat4f& transform, const Ref< IRenderable >& textureSlot, float tiling_factor, const Vec4f& tint_colour )
	{
		constexpr size_t quad_vertex_count = 4;
		const Vec2f* texture_coords = textureSlot->GetTextureCoords();

		if ( sRenderData->quad_index_count >= Renderer2DData::MaxIndices )
			NextBatch();

		float texture_index = 0.0f;
		for ( uint32_t i = 1; i < sRenderData->texture_slot_index; i++ )
		{
			if ( *sRenderData->texture_slots[ i ] == *textureSlot )
			{
				texture_index = ( float )i;
				break;
			}
		}

		if ( texture_index == 0.0f )
		{
			if ( sRenderData->texture_slot_index >= Renderer2DData::MaxTextureSlots )
				NextBatch();

			texture_index = ( float )sRenderData->texture_slot_index;
			sRenderData->texture_slots[ sRenderData->texture_slot_index ] = textureSlot;
			sRenderData->texture_slot_index++;
		}

		for ( size_t i = 0; i < quad_vertex_count; i++ )
		{
			sRenderData->quad_vertex_buffer_ptr->position = transform * Renderer2DData::QuadVertexPositions[ i ];
			sRenderData->quad_vertex_buffer_ptr->colour = tint_colour;
			sRenderData->quad_vertex_buffer_ptr->texCoord = texture_coords[ i ];
			sRenderData->quad_vertex_buffer_ptr->texture_index = texture_index;
			sRenderData->quad_vertex_buffer_ptr->tiling_factor = tiling_factor;
			sRenderData->quad_vertex_buffer_ptr++;
		}

		sRenderData->quad_index_count += 6;
		sRenderData->stats.quad_count++;
	}

	void Renderer2D::DrawLine( const Vec3f& p0, const Vec3f& p1, const Vec4f& colour )
	{
		sRenderData->line_vertex_buffer_ptr->position = p0;
		sRenderData->line_vertex_buffer_ptr->colour = colour;
		sRenderData->line_vertex_buffer_ptr++;

		sRenderData->line_vertex_buffer_ptr->position = p1;
		sRenderData->line_vertex_buffer_ptr->colour = colour;
		sRenderData->line_vertex_buffer_ptr++;

		sRenderData->line_vertex_count += 2;
	}

	void Renderer2D::DrawRect( const Vec2f& position, const Vec2f& size, const Vec4f& colour )
	{
		Mat4f transform = glm::translate( Mat4f( 1.0f ), { position, 0.0f } ) * glm::scale( Mat4f( 1.0f ), { size.x, size.y, 1.0f } );

		DrawRect( transform, colour );
	}

	void Renderer2D::DrawRect( const Mat4f& transform, const Vec4f& colour )
	{
		Vec3f line_vertices[ 4 ];
		for ( size_t i = 0; i < 4; i++ )
			line_vertices[ i ] = transform * Renderer2DData::QuadVertexPositions[ i ];

		DrawLine( line_vertices[ 0 ], line_vertices[ 1 ], colour );
		DrawLine( line_vertices[ 1 ], line_vertices[ 2 ], colour );
		DrawLine( line_vertices[ 2 ], line_vertices[ 3 ], colour );
		DrawLine( line_vertices[ 3 ], line_vertices[ 0 ], colour );
	}

	void Renderer2D::DrawCircle( const Mat4f& transform, const Vec4f& colour, float thickness )
	{
		if ( sRenderData->circle_index_count >= Renderer2DData::MaxIndices )
			NextBatch();

		for ( size_t i = 0; i < 4; i++ )
		{
			sRenderData->circle_vertex_buffer_ptr->world_position = transform * Renderer2DData::QuadVertexPositions[ i ];
			sRenderData->circle_vertex_buffer_ptr->thickness = thickness;
			sRenderData->circle_vertex_buffer_ptr->local_position = Renderer2DData::QuadVertexPositions[ i ] * 2.0f;
			sRenderData->circle_vertex_buffer_ptr->colour = colour;
			sRenderData->circle_vertex_buffer_ptr++;
		}

		sRenderData->circle_index_count += 6;

		sRenderData->stats.quad_count++;
	}

	void Renderer2D::StartBatch()
	{
		sRenderData->quad_index_count = 0;
		sRenderData->quad_vertex_buffer_ptr = sRenderData->quad_vertex_buffer_base;

		sRenderData->circle_index_count = 0;
		sRenderData->circle_vertex_buffer_ptr = sRenderData->circle_vertex_buffer_base;

		sRenderData->line_vertex_count = 0;
		sRenderData->line_vertex_buffer_ptr = sRenderData->line_vertex_buffer_base;

		sRenderData->texture_slot_index = 1;
	}

	void Renderer2D::Flush()
	{
		// Quads
		if ( sRenderData->quad_index_count )
		{
			uint32_t quad_data_size = ( uint32_t )( ( uint8_t* )sRenderData->quad_vertex_buffer_ptr - ( uint8_t* )sRenderData->quad_vertex_buffer_base );
			sRenderData->quad_vertex_buffer->SetData( sRenderData->quad_vertex_buffer_base, quad_data_size );

			for ( uint32_t i = 0; i < sRenderData->texture_slot_index; i++ )
				sRenderData->texture_slots[ i ]->BindTexture( i );

			sRenderData->quad_shader->Bind();
			Renderer::DrawIndexed( sRenderData->quad_vertex_array, sRenderData->quad_index_count );
			sRenderData->stats.draw_calls++;
		}

		// Circles
		if ( sRenderData->circle_index_count )
		{
			uint32_t circle_data_size = ( uint32_t )( ( uint8_t* )sRenderData->circle_vertex_buffer_ptr - ( uint8_t* )sRenderData->circle_vertex_buffer_base );
			sRenderData->circle_vertex_buffer->SetData( sRenderData->circle_vertex_buffer_base, circle_data_size );

			sRenderData->circle_shader->Bind();
			Renderer::DrawIndexed( sRenderData->circle_vertex_array, sRenderData->circle_index_count );
			sRenderData->stats.draw_calls++;
		}

		// Lines
		if ( sRenderData->line_vertex_count )
		{
			uint32_t line_data_size = ( uint32_t )( ( uint8_t* )sRenderData->line_vertex_buffer_ptr - ( uint8_t* )sRenderData->line_vertex_buffer_base );
			sRenderData->line_vertex_buffer->SetData( sRenderData->line_vertex_buffer_base, line_data_size );

			sRenderData->line_shader->Bind();
			Renderer::SetLineWidth( sRenderData->line_width );
			Renderer::DrawLines( sRenderData->line_vertex_array, sRenderData->line_vertex_count );
			sRenderData->stats.draw_calls++;
		}
	}

	void Renderer2D::NextBatch()
	{
		Flush();
		StartBatch();
	}

	void Renderer2D::ResetStats()
	{
		memset( &sRenderData->stats, 0, sizeof( RenderStatistics ) );
	}

	const RenderStatistics& Renderer2D::GetStats()
	{
		return sRenderData->stats;
	}
} // namespace sl
