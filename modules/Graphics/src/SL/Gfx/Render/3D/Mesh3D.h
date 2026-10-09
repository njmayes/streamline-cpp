#pragma once

#include "Bounds3D.h"
#include "SL/Gfx/Render/VertexArray.h"

#include <cstdint>
#include <vector>

namespace sl {

	struct MeshVertex3D
	{
		Vec3f position{ 0.0f };
		Vec3f normal{ 0.0f, 1.0f, 0.0f };
		Vec2f texCoord{ 0.0f };
		Vec4f tangent{ 1.0f, 0.0f, 0.0f, 1.0f };
	};

	class Mesh3D : public RefCounted
	{
	public:
		Mesh3D( const std::vector< MeshVertex3D >& vertices, const std::vector< uint32_t >& indices );
		const Ref< VertexArray >& GetVertexArray() const { return mVertexArray; }
		const Bounds3D& GetBounds() const { return mBounds; }
		uint32_t GetIndexCount() const { return mIndexCount; }

	private:
		Ref< VertexArray > mVertexArray;
		Bounds3D mBounds;
		uint32_t mIndexCount = 0;
	};

	Ref< Mesh3D > CreateCubeMesh();
	Ref< Mesh3D > CreatePlaneMesh( float width = 1.0f, float depth = 1.0f );
	Ref< Mesh3D > CreateSphereMesh( uint32_t segments = 32, uint32_t rings = 16 );

} // namespace sl
