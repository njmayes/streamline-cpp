#pragma once

#include "Camera3D.h"
#include "Light3D.h"
#include "Material3D.h"
#include "Mesh3D.h"
#include "Skybox3D.h"
#include "Transform3D.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace sl {

	struct MeshInstance3D
	{
		Ref< Mesh3D > mesh;
		Material3D material;
		bool castShadows = true;
		bool visible = true;
	};

	class SceneNode3D
	{
	public:
		std::string name;
		Transform3D transform;
		std::vector< MeshInstance3D > instances;
		bool visible = true;

		SceneNode3D& AddChild( std::string childName = {} );
		SceneNode3D* GetParent() const { return mParent; }
		Mat4f GetWorldMatrix() const;
		void Traverse( const std::function< void( const SceneNode3D&, const Mat4f& ) >& visitor, const Mat4f& parent = Mat4f( 1.0f ) ) const;

	private:
		friend class Scene3D;
		explicit SceneNode3D( std::string name, SceneNode3D* parent ) : name( std::move( name ) ), mParent( parent ) {}
		SceneNode3D* mParent = nullptr;
		std::vector< std::unique_ptr< SceneNode3D > > mChildren;
	};

	class Scene3D
	{
	public:
		SceneNode3D& CreateNode( std::string name = {} );
		void Clear();
		void Traverse( const std::function< void( const SceneNode3D&, const Mat4f& ) >& visitor ) const;

		Vec3f ambient{ 0.03f };
		Ref< Skybox3D > skybox;
		DirectionalLight3D sun;
		std::vector< PointLight3D > pointLights;
		std::vector< SpotLight3D > spotLights;

	private:
		std::vector< std::unique_ptr< SceneNode3D > > mRoots;
	};

} // namespace sl
