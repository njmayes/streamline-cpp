#pragma once

#include "Bounds3D.h"
#include "Camera3D.h"
#include "HDRTarget3D.h"
#include "Scene3D.h"
#include "ShadowMap3D.h"

#include "SL/Gfx/Render/Shader.h"

#include <filesystem>
#include <memory>
#include <vector>

namespace sl {

	struct Renderer3DSettings
	{
		std::filesystem::path shaderDirectory = "resources/shaders";
		uint32_t width = 1280;
		uint32_t height = 720;
		uint32_t shadowResolution = 2048;
		float shadowDistance = 40.0f;
		float exposure = 1.0f;
		float gamma = 2.2f;
		Vec4f clearColour{ 0.025f, 0.04f, 0.07f, 1.0f };
		bool shadows = true;
		bool hdr = true;
		bool frustumCulling = true;
		bool fxaa = true;
	};

	struct Renderer3DStatistics
	{
		uint32_t submitted = 0;
		uint32_t visible = 0;
		uint32_t culled = 0;
		uint32_t drawCalls = 0;
		uint32_t shadowDrawCalls = 0;
		uint32_t triangles = 0;
	};

	// Initialize and render on the OpenGL context's owning thread.
	// Call RenderScene after Renderer::Init(), before Renderer::Shutdown().
	class Renderer3D
	{
	public:
		static void Init( const Renderer3DSettings& settings = {} );
		static void Shutdown();
		static void Resize( uint32_t width, uint32_t height );
		static void SetSettings( const Renderer3DSettings& settings );
		static const Renderer3DSettings& GetSettings();
		static const Renderer3DStatistics& GetStats();
		static void RenderScene( const Scene3D& scene, const Camera3D& camera );

		// Optional immediate-style usage: submit geometry between BeginState/EndState.
		static void BeginState( const Camera3D& camera, const Scene3D& lighting );
		static void DrawMesh( const Ref< Mesh3D >& mesh, const Mat4f& world, const Material3D& material = {}, bool castShadows = true );
		static void EndState();

	private:
		struct DrawItem
		{
			Ref< Mesh3D > mesh;
			Mat4f world{ 1.0f };
			Material3D material;
			bool castShadows = true;
			float distanceSquared = 0.0f;
		};
		struct State;
		static void Submit( const DrawItem& item );
		static void Execute();
		static void RenderShadowPass();
		static void RenderForwardPass();
		static void DrawSurface( const DrawItem& item );
		static void RenderComposite();
		static void RenderSkybox();
		static std::unique_ptr< State > sState;
	};

} // namespace sl
