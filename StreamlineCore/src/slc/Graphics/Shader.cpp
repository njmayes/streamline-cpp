#include "Shader.h"

#include <fstream>

#include <glad/glad.h>

#include <glm/gtc/type_ptr.hpp>

#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_glsl.hpp>

#include "slc/Filesystem/Utils.h"
#include "slc/Logging/Log.h"
#include "slc/Types/Timer.h"

namespace slc {

	namespace Utils {

		static GLenum ShaderTypeFromString( std::string_view type )
		{
			if ( type == "vertex" )
				return GL_VERTEX_SHADER;
			if ( type == "fragment" || type == "pixel" )
				return GL_FRAGMENT_SHADER;

			ASSERT( false, "Unknown shader type!" );
			return 0;
		}

		static shaderc_shader_kind GLShaderStageToShaderC( GLenum stage )
		{
			switch ( stage )
			{
				case GL_VERTEX_SHADER:
					return shaderc_glsl_vertex_shader;
				case GL_FRAGMENT_SHADER:
					return shaderc_glsl_fragment_shader;
			}
			ASSERT( false );
			return ( shaderc_shader_kind )0;
		}

		static const char* GLShaderStageToString( GLenum stage )
		{
			switch ( stage )
			{
				case GL_VERTEX_SHADER:
					return "GL_VERTEX_SHADER";
				case GL_FRAGMENT_SHADER:
					return "GL_FRAGMENT_SHADER";
			}
			ASSERT( false );
			return nullptr;
		}

		static const char* GetCacheDirectory()
		{
			// TODO: make sure the assets directory is valid
			return "resources/cache/shader/opengl";
		}

		static void CreateCacheDirectoryIfNeeded()
		{
			std::string cache_directory = GetCacheDirectory();
			if ( !std::filesystem::exists( cache_directory ) )
				std::filesystem::create_directories( cache_directory );
		}

		static const char* GLShaderStageCachedOpenGLFileExtension( uint32_t stage )
		{
			switch ( stage )
			{
				case GL_VERTEX_SHADER:
					return ".cached_opengl.vert";
				case GL_FRAGMENT_SHADER:
					return ".cached_opengl.frag";
			}
			ASSERT( false );
			return "";
		}

		static const char* GLShaderStageCachedVulkanFileExtension( uint32_t stage )
		{
			switch ( stage )
			{
				case GL_VERTEX_SHADER:
					return ".cached_vulkan.vert";
				case GL_FRAGMENT_SHADER:
					return ".cached_vulkan.frag";
			}
			ASSERT( false );
			return "";
		}
	} // namespace Utils

	Shader::Shader( std::string_view filepath )
		: mFilepath( filepath )
	{
		Utils::CreateCacheDirectoryIfNeeded();

		std::string source = fs::ReadToString( filepath );
		auto shader_sources = PreProcess( source );

		{
			Timer timer;
			CompileOrGetVulkanBinaries( shader_sources );
			CompileOrGetOpenGLBinaries();
			CreateProgram();
			log::Info( "Shader creation took {0} ms", timer.ElapsedMillis() );
		}

		// Extract name from file path
		auto name_start = filepath.find_last_of( '/\\' );
		name_start = name_start == std::string::npos ? 0 : name_start + 1;
		auto name_end = filepath.rfind( '.' );
		auto count = ( name_end == std::string::npos ) ? filepath.size() - name_end : name_end - name_start;
		mName = filepath.substr( name_start, count );
	}

	Shader::Shader( std::string_view name, std::string_view vertex_src, std::string_view fragment_src )
		: mName( name )
	{
		std::unordered_map< GLenum, std::string > sources;
		sources[ GL_VERTEX_SHADER ] = vertex_src;
		sources[ GL_FRAGMENT_SHADER ] = fragment_src;

		CompileOrGetVulkanBinaries( sources );
		CompileOrGetOpenGLBinaries();
		CreateProgram();
	}

	Shader::~Shader()
	{
		glDeleteProgram( mRendererID );
	}

	std::unordered_map< GLenum, std::string > Shader::PreProcess( std::string_view source )
	{
		std::unordered_map< GLenum, std::string > shader_sources;

		const char* type_token = "#type";
		size_t type_token_length = strlen( type_token );
		size_t pos = source.find( type_token, 0 );
		while ( pos != std::string::npos )
		{
			size_t eol = source.find_first_of( "\r\n", pos );
			ASSERT( eol != std::string::npos, "Shader syntax error!" );
			size_t begin = pos + type_token_length + 1;
			std::string type( source.substr( begin, eol - begin ) );
			ASSERT( Utils::ShaderTypeFromString( type ), "Invalid shader type given!" );

			size_t nextLinePos = source.find_first_not_of( "\r\n", eol );
			ASSERT( nextLinePos != std::string::npos, "Shader syntax error!" );
			pos = source.find( type_token, nextLinePos );
			shader_sources[ Utils::ShaderTypeFromString( type ) ] = ( pos == std::string::npos ) ? source.substr( nextLinePos ) : source.substr( nextLinePos, pos - nextLinePos );
		}

		return shader_sources;
	}

	void Shader::CompileOrGetVulkanBinaries( const std::unordered_map< GLenum, std::string >& shader_sources )
	{
		GLuint program = glCreateProgram();
		shaderc::Compiler compiler;
		shaderc::CompileOptions options;
		options.SetTargetEnvironment( shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2 );
		const bool optimize = true;
		if ( optimize )
			options.SetOptimizationLevel( shaderc_optimization_level_performance );

		std::filesystem::path cache_directory = Utils::GetCacheDirectory();

		auto& shader_data = mVulkanSPIRV;
		shader_data.clear();
		for ( auto&& [ stage, source ] : shader_sources )
		{
			std::filesystem::path shader_filepath = mFilepath;
			std::filesystem::path cached_path = cache_directory / ( shader_filepath.filename().string() + Utils::GLShaderStageCachedVulkanFileExtension( stage ) );

			std::ifstream in( cached_path, std::ios::in | std::ios::binary );
			if ( in.is_open() )
			{
				in.seekg( 0, std::ios::end );
				auto size = in.tellg();
				in.seekg( 0, std::ios::beg );

				auto& data = shader_data[ stage ];
				data.resize( size / sizeof( uint32_t ) );
				in.read( ( char* )data.data(), size );
			}
			else
			{
				shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv( source, Utils::GLShaderStageToShaderC( stage ), mFilepath.c_str(), options );
				if ( module.GetCompilationStatus() != shaderc_compilation_status_success )
				{
					log::Error( module.GetErrorMessage() );
					ASSERT( false );
				}

				shader_data[ stage ] = std::vector< uint32_t >( module.cbegin(), module.cend() );

				std::ofstream out( cached_path, std::ios::out | std::ios::binary );
				if ( out.is_open() )
				{
					auto& data = shader_data[ stage ];
					out.write( ( char* )data.data(), data.size() * sizeof( uint32_t ) );
					out.flush();
					out.close();
				}
			}
		}

		for ( auto&& [ stage, data ] : shader_data )
			Reflect( stage, data );
	}

	void Shader::CompileOrGetOpenGLBinaries()
	{
		auto& shader_data = mOpenGLSPIRV;

		shaderc::Compiler compiler;
		shaderc::CompileOptions options;
		options.SetTargetEnvironment( shaderc_target_env_opengl, shaderc_env_version_opengl_4_5 );
		const bool optimize = false;
		if ( optimize )
			options.SetOptimizationLevel( shaderc_optimization_level_performance );

		std::filesystem::path cache_directory = Utils::GetCacheDirectory();

		shader_data.clear();
		mOpenGLSourceCode.clear();
		for ( auto&& [ stage, spirv ] : mVulkanSPIRV )
		{
			std::filesystem::path shader_filepath = mFilepath;
			std::filesystem::path cached_path = cache_directory / ( shader_filepath.filename().string() + Utils::GLShaderStageCachedOpenGLFileExtension( stage ) );

			std::ifstream in( cached_path, std::ios::in | std::ios::binary );
			if ( in.is_open() )
			{
				in.seekg( 0, std::ios::end );
				auto size = in.tellg();
				in.seekg( 0, std::ios::beg );

				auto& data = shader_data[ stage ];
				data.resize( size / sizeof( uint32_t ) );
				in.read( ( char* )data.data(), size );
			}
			else
			{
				spirv_cross::CompilerGLSL glsl_compiler( spirv );
				mOpenGLSourceCode[ stage ] = glsl_compiler.compile();
				auto& source = mOpenGLSourceCode[ stage ];

				shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv( source, Utils::GLShaderStageToShaderC( stage ), mFilepath.c_str() );
				if ( module.GetCompilationStatus() != shaderc_compilation_status_success )
				{
					log::Error( module.GetErrorMessage() );
					ASSERT( false );
				}

				shader_data[ stage ] = std::vector< uint32_t >( module.cbegin(), module.cend() );

				std::ofstream out( cached_path, std::ios::out | std::ios::binary );
				if ( out.is_open() )
				{
					auto& data = shader_data[ stage ];
					out.write( ( char* )data.data(), data.size() * sizeof( uint32_t ) );
					out.flush();
					out.close();
				}
			}
		}
	}

	void Shader::CreateProgram()
	{
		GLuint program = glCreateProgram();

		std::vector< GLuint > shader_ids;
		for ( auto&& [ stage, spirv ] : mOpenGLSPIRV )
		{
			GLuint shader_id = shader_ids.emplace_back( glCreateShader( stage ) );
			glShaderBinary( 1, &shader_id, GL_SHADER_BINARY_FORMAT_SPIR_V, spirv.data(), ( uint32_t )( spirv.size() * sizeof( uint32_t ) ) );
			glSpecializeShader( shader_id, "main", 0, nullptr, nullptr );
			glAttachShader( program, shader_id );
		}

		glLinkProgram( program );

		GLint is_linked;
		glGetProgramiv( program, GL_LINK_STATUS, &is_linked );
		if ( is_linked == GL_FALSE )
		{
			GLint max_length;
			glGetProgramiv( program, GL_INFO_LOG_LENGTH, &max_length );

			std::vector< GLchar > infoLog( max_length );
			glGetProgramInfoLog( program, max_length, &max_length, infoLog.data() );
			log::Error( "Shader linking failed ({0}):\n{1}", mFilepath, infoLog.data() );

			glDeleteProgram( program );

			for ( auto id : shader_ids )
				glDeleteShader( id );
		}

		for ( auto id : shader_ids )
		{
			glDetachShader( program, id );
			glDeleteShader( id );
		}

		mRendererID = program;
	}

	void Shader::Reflect( GLenum stage, const std::vector< uint32_t >& shader_data )
	{
		spirv_cross::Compiler compiler( shader_data );
		spirv_cross::ShaderResources resources = compiler.get_shader_resources();

		log::Trace( "Shader::Reflect - {0} {1}", Utils::GLShaderStageToString( stage ), mFilepath );
		log::Trace( "    {0} uniform buffers", resources.uniform_buffers.size() );
		log::Trace( "    {0} resources", resources.sampled_images.size() );

		log::Trace( "Uniform buffers:" );
		for ( const auto& resource : resources.uniform_buffers )
		{
			const auto& bufferType = compiler.get_type( resource.base_type_id );
			size_t bufferSize = compiler.get_declared_struct_size( bufferType );
			uint32_t binding = compiler.get_decoration( resource.id, spv::DecorationBinding );
			size_t memberCount = bufferType.member_types.size();

			log::Trace( "  {0}", resource.name );
			log::Trace( "    Size = {0}", bufferSize );
			log::Trace( "    Binding = {0}", binding );
			log::Trace( "    Members = {0}", memberCount );
		}
	}

	void Shader::Bind() const
	{
		glUseProgram( mRendererID );
	}

	void Shader::Unbind() const
	{
		glUseProgram( 0 );
	}

	void Shader::SetInt( std::string_view name, int value )
	{
		GLint location = GetUniformLocation( name );
		if ( location == -1 )
			return;

		glUniform1i( location, value );
	}

	void Shader::SetIntArray( std::string_view name, int* values, uint32_t count )
	{
		GLint location = glGetUniformLocation( mRendererID, name.data() );
		glUniform1iv( location, count, values );
	}

	void Shader::SetMat4( std::string_view name, const Matrix4f& value )
	{
		GLint location = GetUniformLocation( name );
		if ( location == -1 )
			return;

		glUniformMatrix4fv( location, 1, GL_FALSE, glm::value_ptr( value ) );
	}

	void Shader::SetFloat( std::string_view name, float value )
	{
		GLint location = GetUniformLocation( name );
		if ( location == -1 )
			return;

		glUniform1f( location, value );
	}

	void Shader::SetFloat3( std::string_view name, const Vector3f& value )
	{
		GLint location = GetUniformLocation( name );
		if ( location == -1 )
			return;

		glUniform3f( location, value.x, value.y, value.z );
	}

	void Shader::SetFloat4( std::string_view name, const Vector4f& value )
	{
		GLint location = GetUniformLocation( name );
		if ( location == -1 )
			return;

		glUniform4f( location, value.x, value.y, value.z, value.w );
	}

	GLint Shader::GetUniformLocation( std::string_view name ) const
	{
		if ( not mUniformLocCache.contains( name ) )
			return mUniformLocCache[ name ];

		GLint location = glGetUniformLocation( mRendererID, name.data() );
		if ( location == -1 )
		{
			// Don't store in cache if unable to find uniform.
			log::Warn( "Could not get uniform `{0}` from shader to add to cache!", name );
			return location;
		}

		mUniformLocCache[ name ] = location;
		return location;
	}
} // namespace slc