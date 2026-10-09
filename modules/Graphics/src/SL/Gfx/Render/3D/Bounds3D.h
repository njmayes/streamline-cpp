#pragma once

#include <SL/Core/Types/Math.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace sl {

	struct Bounds3D
	{
		Vec3f min{ std::numeric_limits< float >::max() };
		Vec3f max{ std::numeric_limits< float >::lowest() };

		void Expand( const Vec3f& point )
		{
			min = glm::min( min, point );
			max = glm::max( max, point );
		}

		Vec3f Center() const { return ( min + max ) * 0.5f; }
		float Radius() const { return glm::length( max - Center() ); }
	};

	// Extracted from a clip matrix (GLM uses column-major indexing).
	class Frustum3D
	{
	public:
		explicit Frustum3D( const Mat4f& viewProjection )
		{
			const auto row = [&]( int index ) {
				return Vec4f( viewProjection[ 0 ][ index ], viewProjection[ 1 ][ index ], viewProjection[ 2 ][ index ], viewProjection[ 3 ][ index ] );
			};
			mPlanes = { row( 3 ) + row( 0 ), row( 3 ) - row( 0 ), row( 3 ) + row( 1 ),
				row( 3 ) - row( 1 ), row( 3 ) + row( 2 ), row( 3 ) - row( 2 ) };
			for ( auto& plane : mPlanes )
			{
				const float normalLength = glm::length( Vec3f( plane ) );
				if ( normalLength > 0.0f )
					plane /= normalLength;
			}
		}

		bool Contains( const Bounds3D& bounds, const Mat4f& transform ) const
		{
			const Vec3f center = Vec3f( transform * Vec4f( bounds.Center(), 1.0f ) );
			const Vec3f extent = ( bounds.max - bounds.min ) * 0.5f;
			const Vec3f axis0 = Vec3f( transform[ 0 ] ) * extent.x;
			const Vec3f axis1 = Vec3f( transform[ 1 ] ) * extent.y;
			const Vec3f axis2 = Vec3f( transform[ 2 ] ) * extent.z;
			for ( const auto& plane : mPlanes )
			{
				const Vec3f normal{ plane };
				const float radius = std::abs( glm::dot( normal, axis0 ) ) + std::abs( glm::dot( normal, axis1 ) ) + std::abs( glm::dot( normal, axis2 ) );
				if ( glm::dot( normal, center ) + plane.w + radius < 0.0f )
					return false;
			}
			return true;
		}

	private:
		std::array< Vec4f, 6 > mPlanes;
	};

} // namespace sl
