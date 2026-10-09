#include "SL/Core.h"
#include "SL/Gfx.h"

#include <filesystem>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <imgui.h>

namespace {

	class Graphics3DLayer : public sl::ApplicationLayer
	{
	public:
		void OnAttach() override
		{
			sl::Renderer3D::Init();
			mCamera.LookAt( { 4.0f, 3.0f, 7.0f }, { 0.0f, 0.3f, 0.0f } );
			auto& floor = mScene.CreateNode( "Floor" );
			sl::Material3D floorMaterial;
			floorMaterial.baseColour = { 0.3f, 0.35f, 0.4f, 1.0f };
			floorMaterial.roughness = 0.95f;
			floor.instances.push_back( { sl::CreatePlaneMesh( 20, 20 ), floorMaterial } );

			mCube = &mScene.CreateNode( "Cube" );
			mCube->transform.position = { 0, 0.75f, 0 };
			sl::Material3D cubeMaterial;
			cubeMaterial.baseColour = { 0.95f, 0.22f, 0.12f, 1 };
			cubeMaterial.metallic = 0.45f;
			cubeMaterial.roughness = 0.2f;
			mCube->instances.push_back( { sl::CreateCubeMesh(), cubeMaterial } );

			for ( int i = 0; i < 5; ++i )
			{
				auto& sphere = mScene.CreateNode( "Roughness sphere" );
				sphere.transform.position = { ( i - 2 ) * 1.3f, 0.5f, -2.0f };
				sl::Material3D metal;
				metal.baseColour = { 0.95f, 0.73f, 0.3f, 1.0f };
				metal.metallic = 1.0f;
				metal.roughness = 0.08f + i * 0.2f;
				sphere.instances.push_back( { sl::CreateSphereMesh(), metal } );
			}

			mScene.pointLights.push_back( { { 2.0f, 3.0f, 1.5f }, { 1.0f, 0.6f, 0.4f }, 65.0f, 15.0f } );
			mScene.spotLights.push_back( { { -3.0f, 4.0f, 0.0f }, { 0.6f, -1.0f, 0.0f }, { 0.3f, 0.65f, 1.0f }, 120.0f, 12.0f, 0.3f, 0.5f } );
		}

		void OnUpdate( sl::Timestep ts ) override
		{
			mAngle += static_cast< float >( ts.GetSeconds() ) * 0.5f;
			if ( mCube )
				mCube->transform.rotation = glm::angleAxis( mAngle, sl::Vec3f( 0, 1, 0 ) );
		}

		void OnRender() override
		{
			const auto width = sl::GuiApplication::GetWindowWidth();
			const auto height = sl::GuiApplication::GetWindowHeight();
			if ( width <= 0 || height <= 0 )
				return;
			mCamera.SetPerspective( glm::radians( 55.0f ), width / height, 0.1f, 100.0f );
			sl::Renderer3D::Resize( static_cast< uint32_t >( width ), static_cast< uint32_t >( height ) );
			sl::Renderer3D::RenderScene( mScene, mCamera );
		}

		void OnOverlayRender() override
		{
			ImGui::Begin( "3D engine" );
			const auto& stats = sl::Renderer3D::GetStats();
			ImGui::Text( "Visible meshes: %u (culled: %u)", stats.visible, stats.culled );
			ImGui::Text( "Draw calls: %u, shadow draws: %u", stats.drawCalls, stats.shadowDrawCalls );
			ImGui::Text( "Triangles: %u", stats.triangles );
			auto settings = sl::Renderer3D::GetSettings();
			bool changed = ImGui::Checkbox( "Shadows", &settings.shadows );
			changed |= ImGui::Checkbox( "FXAA", &settings.fxaa );
			changed |= ImGui::SliderFloat( "Exposure", &settings.exposure, 0.1f, 5.0f );
			if ( changed )
				sl::Renderer3D::SetSettings( settings );
			ImGui::End();
		}

	private:
		sl::Scene3D mScene;
		sl::Camera3D mCamera;
		sl::SceneNode3D* mCube = nullptr;
		float mAngle = 0.0f;
	};

} // namespace

int main( int argc, char** argv )
{
	const auto executableDirectory = std::filesystem::absolute( argv[ 0 ] ).parent_path();
	auto launch = [ executableDirectory ]( sl::CommandLineArgs ) -> sl::Box< sl::Application > {
		auto spec = sl::Ref< sl::GuiApplicationSpecification >::Create();
		spec->name = "Streamline 3D Demo";
		spec->working_dir = executableDirectory;
		auto app = sl::MakeBox< sl::GuiApplication >( spec );
		app->AddLogTarget< sl::ConsoleLogTarget >( sl::LogLevel::Info );
		app->PushLayer< Graphics3DLayer >();
		return app;
	};
	sl::Application::Run( launch, sl::CommandLineArgs{ argc, argv } );
}
