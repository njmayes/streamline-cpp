#include "Scene3D.h"

namespace sl {

	SceneNode3D& SceneNode3D::AddChild( std::string childName )
	{
		auto child = std::unique_ptr< SceneNode3D >( new SceneNode3D( std::move( childName ), this ) );
		auto& result = *child;
		mChildren.push_back( std::move( child ) );
		return result;
	}

	Mat4f SceneNode3D::GetWorldMatrix() const
	{
		return mParent ? mParent->GetWorldMatrix() * transform.Matrix() : transform.Matrix();
	}

	void SceneNode3D::Traverse( const std::function< void( const SceneNode3D&, const Mat4f& ) >& visitor, const Mat4f& parent ) const
	{
		if ( !visible )
			return;
		const Mat4f world = parent * transform.Matrix();
		visitor( *this, world );
		for ( const auto& child : mChildren )
			child->Traverse( visitor, world );
	}

	SceneNode3D& Scene3D::CreateNode( std::string name )
	{
		auto node = std::unique_ptr< SceneNode3D >( new SceneNode3D( std::move( name ), nullptr ) );
		auto& result = *node;
		mRoots.push_back( std::move( node ) );
		return result;
	}

	void Scene3D::Clear()
	{
		mRoots.clear();
		pointLights.clear();
		spotLights.clear();
		skybox = nullptr;
	}

	void Scene3D::Traverse( const std::function< void( const SceneNode3D&, const Mat4f& ) >& visitor ) const
	{
		for ( const auto& root : mRoots )
			root->Traverse( visitor );
	}

} // namespace sl
