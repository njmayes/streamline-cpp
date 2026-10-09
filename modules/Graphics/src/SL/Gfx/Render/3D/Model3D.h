#pragma once

#include "Scene3D.h"

#include <filesystem>
#include <vector>

namespace sl {

	struct ModelPart3D
	{
		Ref< Mesh3D > mesh;
		Material3D material;
		std::string name;
	};

	// The loader builds GPU resources; call it on a thread with an active OpenGL context.
	class Model3D : public RefCounted
	{
	public:
		std::vector< ModelPart3D > parts;

		void AddToScene( SceneNode3D& node ) const;
	};

	static Ref< Model3D > LoadOBJ( const std::filesystem::path& path );

} // namespace sl
