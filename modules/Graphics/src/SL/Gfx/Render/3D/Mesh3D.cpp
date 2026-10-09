#include "Mesh3D.h"

#include <cmath>
#include <limits>
#include <numbers>
#include <glm/geometric.hpp>

namespace sl {

	Mesh3D::Mesh3D( const std::vector< MeshVertex3D >& vertices, const std::vector< uint32_t >& indices )
	{
		SL_VERIFY( !vertices.empty() && !indices.empty() && indices.size() % 3 == 0, "Mesh3D requires indexed triangles" );
		SL_VERIFY( vertices.size() <= std::numeric_limits< uint32_t >::max() / ( 12 * sizeof( float ) ), "Mesh3D vertex buffer too large" );
		SL_VERIFY( indices.size() <= std::numeric_limits< uint32_t >::max(), "Mesh3D index buffer too large" );
		for ( const auto& vertex : vertices )
			mBounds.Expand( vertex.position );
		for ( uint32_t index : indices )
			SL_VERIFY( index < vertices.size(), "Mesh3D index out of bounds" );

		mIndexCount = static_cast< uint32_t >( indices.size() );
		// Pack attributes explicitly. GLM vector alignment is configurable, while
		// BufferLayout expects a tightly packed 48-byte GPU vertex.
		std::vector< float > packed;
		packed.reserve( vertices.size() * 12 );
		for ( const auto& vertex : vertices )
		{
			packed.insert( packed.end(), { vertex.position.x, vertex.position.y, vertex.position.z, vertex.normal.x, vertex.normal.y, vertex.normal.z, vertex.texCoord.x, vertex.texCoord.y, vertex.tangent.x, vertex.tangent.y, vertex.tangent.z, vertex.tangent.w } );
		}
		mVertexArray = Ref< VertexArray >::Create();
		auto buffer = Ref< VertexBuffer >::Create( static_cast< uint32_t >( packed.size() * sizeof( float ) ) );
		buffer->SetLayout( { { ShaderDataType::Float3, "aPosition" }, { ShaderDataType::Float3, "aNormal" }, { ShaderDataType::Float2, "aTexCoord" }, { ShaderDataType::Float4, "aTangent" } } );
		buffer->SetData( packed.data(), static_cast< uint32_t >( packed.size() * sizeof( float ) ) );
		mVertexArray->AddVertexBuffer( buffer );
		// Existing IndexBuffer API accepts a non-const pointer but only uploads its contents.
		mVertexArray->SetIndexBuffer( Ref< IndexBuffer >::Create( const_cast< uint32_t* >( indices.data() ), mIndexCount ) );
	}

	Ref< Mesh3D > CreatePlaneMesh( float width, float depth )
	{
		std::vector< MeshVertex3D > vertices( 4 );
		vertices[ 0 ] = { { -width * 0.5f, 0, -depth * 0.5f }, { 0, 1, 0 }, { 0, 0 }, { 1, 0, 0, -1 } };
		vertices[ 1 ] = { { -width * 0.5f, 0, depth * 0.5f }, { 0, 1, 0 }, { 0, 1 }, { 1, 0, 0, -1 } };
		vertices[ 2 ] = { { width * 0.5f, 0, depth * 0.5f }, { 0, 1, 0 }, { 1, 1 }, { 1, 0, 0, -1 } };
		vertices[ 3 ] = { { width * 0.5f, 0, -depth * 0.5f }, { 0, 1, 0 }, { 1, 0 }, { 1, 0, 0, -1 } };
		return Ref< Mesh3D >::Create( vertices, std::vector< uint32_t >{ 0, 1, 2, 2, 3, 0 } );
	}

	Ref< Mesh3D > CreateCubeMesh()
	{
		const Vec3f normals[] = { { 0, 0, 1 }, { 0, 0, -1 }, { 1, 0, 0 }, { -1, 0, 0 }, { 0, 1, 0 }, { 0, -1, 0 } };
		const Vec3f corners[][ 4 ] = {
			{ { -.5f, -.5f, .5f }, { .5f, -.5f, .5f }, { .5f, .5f, .5f }, { -.5f, .5f, .5f } },
			{ { .5f, -.5f, -.5f }, { -.5f, -.5f, -.5f }, { -.5f, .5f, -.5f }, { .5f, .5f, -.5f } },
			{ { .5f, -.5f, .5f }, { .5f, -.5f, -.5f }, { .5f, .5f, -.5f }, { .5f, .5f, .5f } },
			{ { -.5f, -.5f, -.5f }, { -.5f, -.5f, .5f }, { -.5f, .5f, .5f }, { -.5f, .5f, -.5f } },
			{ { -.5f, .5f, .5f }, { .5f, .5f, .5f }, { .5f, .5f, -.5f }, { -.5f, .5f, -.5f } },
			{ { -.5f, -.5f, -.5f }, { .5f, -.5f, -.5f }, { .5f, -.5f, .5f }, { -.5f, -.5f, .5f } }
		};
		const Vec2f uv[] = { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 } };
		std::vector< MeshVertex3D > vertices;
		std::vector< uint32_t > indices;
		vertices.reserve( 24 );
		indices.reserve( 36 );
		for ( uint32_t face = 0; face < 6; ++face )
		{
			const Vec3f tangent = glm::normalize( corners[ face ][ 1 ] - corners[ face ][ 0 ] );
			const Vec3f bitangent = glm::normalize( corners[ face ][ 3 ] - corners[ face ][ 0 ] );
			const float handedness = glm::dot( glm::cross( normals[ face ], tangent ), bitangent ) < 0 ? -1.0f : 1.0f;
			for ( uint32_t i = 0; i < 4; ++i )
				vertices.push_back( { corners[ face ][ i ], normals[ face ], uv[ i ], Vec4f( tangent, handedness ) } );
			const uint32_t offset = face * 4;
			for ( uint32_t i : { 0u, 1u, 2u, 2u, 3u, 0u } )
				indices.push_back( offset + i );
		}
		return Ref< Mesh3D >::Create( vertices, indices );
	}

	Ref< Mesh3D > CreateSphereMesh( uint32_t segments, uint32_t rings )
	{
		segments = std::clamp( segments, 3u, 1024u );
		rings = std::clamp( rings, 2u, 1024u );
		std::vector< MeshVertex3D > vertices;
		std::vector< uint32_t > indices;
		vertices.reserve( static_cast< size_t >( segments + 1 ) * ( rings + 1 ) );
		for ( uint32_t y = 0; y <= rings; ++y )
		{
			const float v = float( y ) / float( rings );
			const float theta = v * std::numbers::pi_v< float >;
			for ( uint32_t x = 0; x <= segments; ++x )
			{
				const float u = float( x ) / float( segments );
				const float phi = u * 2.0f * std::numbers::pi_v< float >;
				const Vec3f normal{ std::sin( theta ) * std::cos( phi ), std::cos( theta ), std::sin( theta ) * std::sin( phi ) };
				const Vec3f tangent{ -std::sin( phi ), 0.0f, std::cos( phi ) };
				vertices.push_back( { normal * 0.5f, normal, { u, 1.0f - v }, Vec4f( tangent, -1.0f ) } );
			}
		}
		for ( uint32_t y = 0; y < rings; ++y )
			for ( uint32_t x = 0; x < segments; ++x )
			{
				const uint32_t a = y * ( segments + 1 ) + x;
				const uint32_t b = a + segments + 1;
				if ( y != 0 )
				{
					indices.push_back( a );
					indices.push_back( a + 1 );
					indices.push_back( b );
				}
				if ( y + 1 != rings )
				{
					indices.push_back( a + 1 );
					indices.push_back( b + 1 );
					indices.push_back( b );
				}
			}
		return Ref< Mesh3D >::Create( vertices, indices );
	}

} // namespace sl
