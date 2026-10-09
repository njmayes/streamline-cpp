#include "Renderer3D.h"

#include <SL/Gfx/Render/UniformBuffer.h>

#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <iterator>
#include <optional>
#include <stdexcept>

namespace sl {

	namespace {

		constexpr uint32_t MaxPointLights = 16;
		constexpr uint32_t MaxSpotLights = 8;

		// GLSL std140 data; all members occupy multiples of 16 bytes.
		struct CameraGPU
		{
			Mat4f viewProjection{ 1.0f };
			Mat4f lightViewProjection{ 1.0f };
			Vec4f cameraPosition{ 0 };
			Vec4f ambient{ 0 };
			Vec4f sunDirectionIntensity{ 0 };
			Vec4f sunColourShadow{ 0 };
			glm::ivec4 lightCounts{ 0 };
			std::array< Vec4f, MaxPointLights * 2 > points{};
			std::array< Vec4f, MaxSpotLights * 4 > spots{};
		};

		struct SurfaceGPU
		{
			Mat4f model{ 1.0f };
			Mat4f normalMatrix{ 1.0f };
			Vec4f baseColour{ 1.0f };
			Vec4f emissiveMetallic{ 0 };
			Vec4f surfaceParams{ 0 };
			Vec4f textureFlags{ 0 };
		};

		struct ShadowGPU
		{
			Mat4f lightViewProjection{ 1.0f };
			Mat4f model{ 1.0f };
			Vec4f alphaParams{ 0 };
		};

		struct CompositeGPU
		{
			Vec4f tone{ 1.0f, 2.2f, 1.0f, 0.0f };
			Vec4f texel{ 1.0f, 1.0f, 0.0f, 0.0f };
		};

		static_assert( sizeof( CameraGPU ) == 16 * ( 8 + 5 + 32 + 32 ) );
		static_assert( sizeof( SurfaceGPU ) == 16 * 12 );
		static_assert( sizeof( ShadowGPU ) == 16 * 9 );

		struct GLStateGuard
		{
			GLint drawFramebuffer = 0, readFramebuffer = 0;
			GLint viewport[ 4 ]{};
			GLint program = 0, vertexArray = 0;
			GLint depthFunc = 0, cullFace = 0, frontFace = 0;
			GLint blendSrcRGB = 0, blendDstRGB = 0, blendSrcAlpha = 0, blendDstAlpha = 0;
			GLfloat clearColour[ 4 ]{};
			GLfloat polygonFactor = 0, polygonUnits = 0;
			GLboolean depthMask = GL_TRUE;
			GLboolean depth = GL_FALSE, blend = GL_FALSE, cull = GL_FALSE, scissor = GL_FALSE, polygonOffset = GL_FALSE;

			GLStateGuard()
			{
				glGetIntegerv( GL_DRAW_FRAMEBUFFER_BINDING, &drawFramebuffer );
				glGetIntegerv( GL_READ_FRAMEBUFFER_BINDING, &readFramebuffer );
				glGetIntegerv( GL_VIEWPORT, viewport );
				glGetIntegerv( GL_CURRENT_PROGRAM, &program );
				glGetIntegerv( GL_VERTEX_ARRAY_BINDING, &vertexArray );
				glGetIntegerv( GL_DEPTH_FUNC, &depthFunc );
				glGetIntegerv( GL_CULL_FACE_MODE, &cullFace );
				glGetIntegerv( GL_FRONT_FACE, &frontFace );
				glGetIntegerv( GL_BLEND_SRC_RGB, &blendSrcRGB );
				glGetIntegerv( GL_BLEND_DST_RGB, &blendDstRGB );
				glGetIntegerv( GL_BLEND_SRC_ALPHA, &blendSrcAlpha );
				glGetIntegerv( GL_BLEND_DST_ALPHA, &blendDstAlpha );
				glGetBooleanv( GL_DEPTH_WRITEMASK, &depthMask );
				glGetFloatv( GL_COLOR_CLEAR_VALUE, clearColour );
				glGetFloatv( GL_POLYGON_OFFSET_FACTOR, &polygonFactor );
				glGetFloatv( GL_POLYGON_OFFSET_UNITS, &polygonUnits );
				depth = glIsEnabled( GL_DEPTH_TEST );
				blend = glIsEnabled( GL_BLEND );
				cull = glIsEnabled( GL_CULL_FACE );
				scissor = glIsEnabled( GL_SCISSOR_TEST );
				polygonOffset = glIsEnabled( GL_POLYGON_OFFSET_FILL );
			}

			~GLStateGuard()
			{
				glBindFramebuffer( GL_DRAW_FRAMEBUFFER, drawFramebuffer );
				glBindFramebuffer( GL_READ_FRAMEBUFFER, readFramebuffer );
				glViewport( viewport[ 0 ], viewport[ 1 ], viewport[ 2 ], viewport[ 3 ] );
				glUseProgram( program );
				glBindVertexArray( vertexArray );
				glDepthFunc( depthFunc );
				glCullFace( cullFace );
				glFrontFace( frontFace );
				glBlendFuncSeparate( blendSrcRGB, blendDstRGB, blendSrcAlpha, blendDstAlpha );
				glDepthMask( depthMask );
				glClearColor( clearColour[ 0 ], clearColour[ 1 ], clearColour[ 2 ], clearColour[ 3 ] );
				glPolygonOffset( polygonFactor, polygonUnits );
				const auto restore = []( GLenum option, GLboolean enabled ) { if ( enabled ) glEnable( option ); else glDisable( option ); };
				restore( GL_DEPTH_TEST, depth );
				restore( GL_BLEND, blend );
				restore( GL_CULL_FACE, cull );
				restore( GL_SCISSOR_TEST, scissor );
				restore( GL_POLYGON_OFFSET_FILL, polygonOffset );
			}
		};

	} // namespace

	struct Renderer3D::State
	{
		Renderer3DSettings settings;
		Renderer3DStatistics stats;
		Ref< Shader > forward;
		Ref< Shader > shadowShader;
		Ref< Shader > composite;
		Ref< Shader > skyboxShader;
		Ref< UniformBuffer > cameraUniforms;
		Ref< UniformBuffer > surfaceUniforms;
		Ref< UniformBuffer > shadowUniforms;
		Ref< UniformBuffer > compositeUniforms;
		Ref< UniformBuffer > skyboxUniforms;
		std::unique_ptr< ShadowMap3D > shadow;
		std::unique_ptr< HDRTarget3D > hdr;
		uint32_t fullscreenVAO = 0;
		const Camera3D* camera = nullptr;
		const Scene3D* scene = nullptr;
		Mat4f lightViewProjection{ 1.0f };
		bool shadowEnabled = false;
		bool active = false;
		GLint originalDrawFramebuffer = 0;
		GLint originalViewport[ 4 ]{};
		std::optional< Frustum3D > frustum;
		std::vector< DrawItem > shadowCasters;
		std::vector< DrawItem > opaque;
		std::vector< DrawItem > transparent;
	};

	std::unique_ptr< Renderer3D::State > Renderer3D::sState;

	void Renderer3D::Init( const Renderer3DSettings& settings )
	{
		if ( sState ) return;
		SL_VERIFY( settings.width > 0 && settings.height > 0, "Renderer3D resolution must be nonzero" );
		sState = std::make_unique< State >();
		auto& state = *sState;
		state.settings = settings;
		std::filesystem::path shaderDirectory = settings.shaderDirectory;
#if defined( STREAMLINE_GFX_DEFAULT_SHADER_DIR )
		if ( !std::filesystem::exists( shaderDirectory / "Renderer3D.glsl" ) )
			shaderDirectory = STREAMLINE_GFX_DEFAULT_SHADER_DIR;
#endif
		state.forward = Ref< Shader >::Create( ( shaderDirectory / "Renderer3D.glsl" ).string() );
		state.shadowShader = Ref< Shader >::Create( ( shaderDirectory / "Renderer3DShadow.glsl" ).string() );
		state.composite = Ref< Shader >::Create( ( shaderDirectory / "Renderer3DComposite.glsl" ).string() );
		state.skyboxShader = Ref< Shader >::Create( ( shaderDirectory / "Renderer3DSkybox.glsl" ).string() );
		state.cameraUniforms = Ref< UniformBuffer >::Create( sizeof( CameraGPU ), 5 );
		state.surfaceUniforms = Ref< UniformBuffer >::Create( sizeof( SurfaceGPU ), 6 );
		state.shadowUniforms = Ref< UniformBuffer >::Create( sizeof( ShadowGPU ), 7 );
		state.compositeUniforms = Ref< UniformBuffer >::Create( sizeof( CompositeGPU ), 8 );
		state.skyboxUniforms = Ref< UniformBuffer >::Create( sizeof( Mat4f ), 9 );
		state.shadow = std::make_unique< ShadowMap3D >( settings.shadowResolution );
		state.hdr = std::make_unique< HDRTarget3D >( settings.width, settings.height );
		glCreateVertexArrays( 1, &state.fullscreenVAO );
	}

	void Renderer3D::Shutdown()
	{
		if ( !sState ) return;
		glDeleteVertexArrays( 1, &sState->fullscreenVAO );
		sState.reset();
	}

	void Renderer3D::Resize( uint32_t width, uint32_t height )
	{
		if ( !sState || !width || !height ) return;
		sState->settings.width = width;
		sState->settings.height = height;
		sState->hdr->Resize( width, height );
	}

	void Renderer3D::SetSettings( const Renderer3DSettings& settings )
	{
		if ( !sState ) return;
		sState->settings = settings;
		if ( settings.width && settings.height ) sState->hdr->Resize( settings.width, settings.height );
		if ( settings.shadowResolution ) sState->shadow->Resize( settings.shadowResolution );
	}

	const Renderer3DSettings& Renderer3D::GetSettings()
	{
		SL_VERIFY( sState != nullptr, "Renderer3D not initialized" );
		return sState->settings;
	}

	const Renderer3DStatistics& Renderer3D::GetStats()
	{
		SL_VERIFY( sState != nullptr, "Renderer3D not initialized" );
		return sState->stats;
	}

	void Renderer3D::BeginState( const Camera3D& camera, const Scene3D& lighting )
	{
		if ( !sState ) Init();
		SL_VERIFY( sState && !sState->active, "Renderer3D::BeginState requires an initialized inactive renderer" );
		auto& state = *sState;
		state.active = true;
		state.camera = &camera;
		state.scene = &lighting;
		state.stats = {};
		state.opaque.clear();
		state.transparent.clear();
		state.shadowCasters.clear();
		state.frustum.emplace( camera.GetViewProjection() );
	}

	void Renderer3D::Submit( const DrawItem& item )
	{
		auto& state = *sState;
		if ( !item.mesh ) return;
		++state.stats.submitted;
		if ( item.castShadows && item.material.alphaMode != AlphaMode3D::Blend ) state.shadowCasters.push_back( item );
		if ( state.settings.frustumCulling && !state.frustum->Contains( item.mesh->GetBounds(), item.world ) )
		{
			++state.stats.culled;
			return;
		}
		++state.stats.visible;
		const auto position = Vec3f( item.world[ 3 ] );
		DrawItem command = item;
		command.distanceSquared = glm::dot( position - state.camera->GetPosition(), position - state.camera->GetPosition() );
		if ( item.material.alphaMode == AlphaMode3D::Blend ) state.transparent.push_back( std::move( command ) );
		else state.opaque.push_back( std::move( command ) );
	}

	void Renderer3D::DrawMesh( const Ref< Mesh3D >& mesh, const Mat4f& world, const Material3D& material, bool castShadows )
	{
		SL_VERIFY( sState && sState->active, "Renderer3D::DrawMesh must be called inside BeginState/EndState" );
		Submit( { mesh, world, material, castShadows } );
	}

	void Renderer3D::RenderScene( const Scene3D& scene, const Camera3D& camera )
	{
		BeginState( camera, scene );
		scene.Traverse( []( const SceneNode3D& node, const Mat4f& world ) {
			for ( const auto& instance : node.instances )
				if ( instance.visible ) Renderer3D::DrawMesh( instance.mesh, world, instance.material, instance.castShadows );
		} );
		EndState();
	}

	void Renderer3D::EndState()
	{
		SL_VERIFY( sState && sState->active, "Renderer3D::EndState requires an active state" );
		Execute();
		sState->camera = nullptr;
		sState->scene = nullptr;
		sState->active = false;
	}

	void Renderer3D::Execute()
	{
		GLStateGuard guard;
		auto& state = *sState;
		state.originalDrawFramebuffer = guard.drawFramebuffer;
		std::copy( std::begin( guard.viewport ), std::end( guard.viewport ), state.originalViewport );
		glDisable( GL_SCISSOR_TEST );
		glEnable( GL_DEPTH_TEST );
		glDepthFunc( GL_LESS );
		glDepthMask( GL_TRUE );
		glEnable( GL_CULL_FACE );
		glCullFace( GL_BACK );
		glFrontFace( GL_CCW );
		glDisable( GL_BLEND );

		const auto& sunlight = state.scene->sun;
		state.shadowEnabled = state.settings.shadows && sunlight.castShadows && sunlight.intensity > 0 && state.settings.shadowResolution > 0;
		if ( state.shadowEnabled )
		{
			const Vec3f direction = glm::length( sunlight.direction ) > .0001f ? glm::normalize( sunlight.direction ) : Vec3f( 0, -1, 0 );
			const float distance = std::max( state.settings.shadowDistance, 1.0f );
			const Vec3f center = state.camera->GetPosition();
			const Vec3f eye = center - direction * distance;
			const Vec3f up = std::abs( direction.y ) > 0.99f ? Vec3f( 0, 0, 1 ) : Vec3f( 0, 1, 0 );
			state.lightViewProjection = glm::ortho( -distance, distance, -distance, distance, 0.1f, distance * 3.0f ) * glm::lookAt( eye, center, up );
			RenderShadowPass();
		}
		else state.lightViewProjection = Mat4f( 1.0f );

		RenderForwardPass();
		if ( state.settings.hdr )
		{
			glBindFramebuffer( GL_DRAW_FRAMEBUFFER, guard.drawFramebuffer );
			glViewport( guard.viewport[ 0 ], guard.viewport[ 1 ], guard.viewport[ 2 ], guard.viewport[ 3 ] );
			RenderComposite();
		}
	}

	void Renderer3D::RenderShadowPass()
	{
		auto& state = *sState;
		state.shadow->Bind();
		glClear( GL_DEPTH_BUFFER_BIT );
		glEnable( GL_POLYGON_OFFSET_FILL );
		glPolygonOffset( 2.0f, 4.0f );
		glCullFace( GL_FRONT );
		state.shadowShader->Bind();
		const auto draw = [&]( const DrawItem& item ) {
			if ( !item.castShadows || item.material.alphaMode == AlphaMode3D::Blend ) return;
			ShadowGPU data;
			data.lightViewProjection = state.lightViewProjection;
			data.model = item.world;
			data.alphaParams = { item.material.baseColour.a, item.material.alphaCutoff,
				item.material.alphaMode == AlphaMode3D::Mask ? 1.0f : 0.0f, item.material.baseColourTexture ? 1.0f : 0.0f };
			state.shadowUniforms->SetData( &data, sizeof( data ) );
			glBindTextureUnit( 0, item.material.baseColourTexture ? item.material.baseColourTexture->GetTextureID() : 0 );
			if ( item.material.doubleSided ) glDisable( GL_CULL_FACE ); else glEnable( GL_CULL_FACE );
			item.mesh->GetVertexArray()->Bind();
			glDrawElements( GL_TRIANGLES, item.mesh->GetIndexCount(), GL_UNSIGNED_INT, nullptr );
			++state.stats.shadowDrawCalls;
		};
		for ( const auto& item : state.shadowCasters ) draw( item );
		glDisable( GL_POLYGON_OFFSET_FILL );
		glCullFace( GL_BACK );
		glEnable( GL_CULL_FACE );
	}

	void Renderer3D::RenderForwardPass()
	{
		auto& state = *sState;
		if ( state.settings.hdr ) state.hdr->Bind();
		else
		{
			// Keep the caller's framebuffer and viewport when HDR is disabled.
			// ShadowMap3D::Bind changed the draw framebuffer; the caller's binding is
			// restored by Execute's guard, but must be restored here before drawing.
			// The desired draw binding is retained at the start of Execute (see below).
			// This branch is handled by a saved draw framebuffer member in State.
			glBindFramebuffer( GL_DRAW_FRAMEBUFFER, state.originalDrawFramebuffer );
			glViewport( state.originalViewport[ 0 ], state.originalViewport[ 1 ], state.originalViewport[ 2 ], state.originalViewport[ 3 ] );
		}
		glDepthMask( GL_TRUE );
		glClearColor( state.settings.clearColour.r, state.settings.clearColour.g, state.settings.clearColour.b, state.settings.clearColour.a );
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

		CameraGPU data;
		data.viewProjection = state.camera->GetViewProjection();
		data.lightViewProjection = state.lightViewProjection;
		data.cameraPosition = Vec4f( state.camera->GetPosition(), 1.0f );
		data.ambient = Vec4f( state.scene->ambient, 1.0f );
		const auto& sun = state.scene->sun;
		data.sunDirectionIntensity = Vec4f( glm::length( sun.direction ) > .0001f ? glm::normalize( sun.direction ) : Vec3f( 0, -1, 0 ), sun.intensity );
		data.sunColourShadow = Vec4f( sun.colour, state.shadowEnabled ? 1.0f : 0.0f );
		data.lightCounts.x = std::min( static_cast< int >( state.scene->pointLights.size() ), static_cast< int >( MaxPointLights ) );
		data.lightCounts.y = std::min( static_cast< int >( state.scene->spotLights.size() ), static_cast< int >( MaxSpotLights ) );
		for ( int i = 0; i < data.lightCounts.x; ++i )
		{
			const auto& light = state.scene->pointLights[ i ];
			data.points[ i * 2 ] = Vec4f( light.position, light.range );
			data.points[ i * 2 + 1 ] = Vec4f( light.colour, light.intensity );
		}
		for ( int i = 0; i < data.lightCounts.y; ++i )
		{
			const auto& light = state.scene->spotLights[ i ];
			data.spots[ i * 4 ] = Vec4f( light.position, light.range );
			data.spots[ i * 4 + 1 ] = Vec4f( light.direction, 0.0f );
			data.spots[ i * 4 + 2 ] = Vec4f( light.colour, light.intensity );
			data.spots[ i * 4 + 3 ] = Vec4f( std::cos( light.innerAngleRadians ), std::cos( light.outerAngleRadians ), 0, 0 );
		}
		state.cameraUniforms->SetData( &data, sizeof( data ) );
		glBindTextureUnit( 4, state.shadow->GetTextureID() );
		state.forward->Bind();
		std::sort( state.opaque.begin(), state.opaque.end(), []( const DrawItem& a, const DrawItem& b ) { return a.distanceSquared < b.distanceSquared; } );
		for ( const auto& item : state.opaque ) DrawSurface( item );
		if ( state.scene->skybox ) RenderSkybox();
		state.forward->Bind();
		std::sort( state.transparent.begin(), state.transparent.end(), []( const DrawItem& a, const DrawItem& b ) { return a.distanceSquared > b.distanceSquared; } );
		glEnable( GL_BLEND );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		glDepthMask( GL_FALSE );
		for ( const auto& item : state.transparent ) DrawSurface( item );
		glDepthMask( GL_TRUE );
		glDisable( GL_BLEND );
	}

	void Renderer3D::DrawSurface( const DrawItem& item )
	{
		auto& state = *sState;
		const auto& material = item.material;
		SurfaceGPU data;
		data.model = item.world;
		const float determinant = glm::determinant( item.world );
		data.normalMatrix = std::abs( determinant ) > 1e-8f ? glm::transpose( glm::inverse( item.world ) ) : Mat4f( 1.0f );
		data.baseColour = material.baseColour;
		data.emissiveMetallic = Vec4f( material.emissive, material.metallic );
		data.surfaceParams = { material.roughness, material.alphaCutoff, material.alphaMode == AlphaMode3D::Mask ? 1.0f : 0.0f, material.normalScale };
		data.textureFlags = { material.baseColourTexture ? 1.0f : 0.0f, material.normalTexture ? 1.0f : 0.0f,
			material.metallicRoughnessTexture ? 1.0f : 0.0f, material.emissiveTexture ? 1.0f : 0.0f };
		state.surfaceUniforms->SetData( &data, sizeof( data ) );
		glBindTextureUnit( 0, material.baseColourTexture ? material.baseColourTexture->GetTextureID() : 0 );
		glBindTextureUnit( 1, material.normalTexture ? material.normalTexture->GetTextureID() : 0 );
		glBindTextureUnit( 2, material.metallicRoughnessTexture ? material.metallicRoughnessTexture->GetTextureID() : 0 );
		glBindTextureUnit( 3, material.emissiveTexture ? material.emissiveTexture->GetTextureID() : 0 );
		if ( material.doubleSided ) glDisable( GL_CULL_FACE ); else glEnable( GL_CULL_FACE );
		item.mesh->GetVertexArray()->Bind();
		glDrawElements( GL_TRIANGLES, item.mesh->GetIndexCount(), GL_UNSIGNED_INT, nullptr );
		++state.stats.drawCalls;
		state.stats.triangles += item.mesh->GetIndexCount() / 3;
	}

	void Renderer3D::RenderSkybox()
	{
		auto& state = *sState;
		const Mat4f rotationView = Mat4f( glm::mat3( state.camera->GetView() ) );
		const Mat4f inverseVP = glm::inverse( state.camera->GetProjection() * rotationView );
		state.skyboxUniforms->SetData( &inverseVP, sizeof( inverseVP ) );
		state.skyboxShader->Bind();
		glBindTextureUnit( 0, state.scene->skybox->GetTextureID() );
		glDepthFunc( GL_LEQUAL );
		glDepthMask( GL_FALSE );
		glDisable( GL_CULL_FACE );
		glBindVertexArray( state.fullscreenVAO );
		glDrawArrays( GL_TRIANGLES, 0, 3 );
		glDepthFunc( GL_LESS );
		glDepthMask( GL_TRUE );
		glEnable( GL_CULL_FACE );
	}

	void Renderer3D::RenderComposite()
	{
		auto& state = *sState;
		glDisable( GL_DEPTH_TEST );
		glDisable( GL_BLEND );
		glDisable( GL_CULL_FACE );
		CompositeGPU data;
		data.tone = { state.settings.exposure, state.settings.gamma, state.settings.fxaa ? 1.0f : 0.0f, 0.0f };
		data.texel = { 1.0f / state.hdr->GetWidth(), 1.0f / state.hdr->GetHeight(), 0, 0 };
		state.compositeUniforms->SetData( &data, sizeof( data ) );
		glBindTextureUnit( 0, state.hdr->GetColourTexture() );
		state.composite->Bind();
		glBindVertexArray( state.fullscreenVAO );
		glDrawArrays( GL_TRIANGLES, 0, 3 );
	}

} // namespace sl
