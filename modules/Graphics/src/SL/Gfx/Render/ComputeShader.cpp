#include "ComputeShader.h"

#include <glad/gl.h>

#include <vector>

namespace sl {

	bool ComputeShader::IsSupported()
	{
		int major = 0, minor = 0;
		glGetIntegerv( GL_MAJOR_VERSION, &major );
		glGetIntegerv( GL_MINOR_VERSION, &minor );
		return major > 4 || ( major == 4 && minor >= 5 );
	}

	ComputeShader::ComputeShader( std::string_view name, std::string_view source )
	{
		if ( !IsSupported() )
		{
			mError = "OpenGL 4.5 or newer is required for GPU simulation";
			return;
		}

		std::string source_copy( source );
		char const* source_pointer = source_copy.c_str();
		std::uint32_t shader = glCreateShader( GL_COMPUTE_SHADER );
		glShaderSource( shader, 1, &source_pointer, nullptr );
		glCompileShader( shader );

		int status = GL_FALSE;
		glGetShaderiv( shader, GL_COMPILE_STATUS, &status );
		if ( status != GL_TRUE )
		{
			int length = 0;
			glGetShaderiv( shader, GL_INFO_LOG_LENGTH, &length );
			std::vector< char > message( static_cast< std::size_t >( length ) + 1, '\0' );
			glGetShaderInfoLog( shader, length, nullptr, message.data() );
			mError = std::string( name ) + ": " + message.data();
			glDeleteShader( shader );
			return;
		}

		mProgram = glCreateProgram();
		glAttachShader( mProgram, shader );
		glLinkProgram( mProgram );
		glDetachShader( mProgram, shader );
		glDeleteShader( shader );

		glGetProgramiv( mProgram, GL_LINK_STATUS, &status );
		if ( status != GL_TRUE )
		{
			int length = 0;
			glGetProgramiv( mProgram, GL_INFO_LOG_LENGTH, &length );
			std::vector< char > message( static_cast< std::size_t >( length ) + 1, '\0' );
			glGetProgramInfoLog( mProgram, length, nullptr, message.data() );
			mError = std::string( name ) + ": " + message.data();
			glDeleteProgram( mProgram );
			mProgram = 0;
		}
	}

	ComputeShader::~ComputeShader()
	{
		if ( mProgram ) glDeleteProgram( mProgram );
	}

	void ComputeShader::Bind() const { glUseProgram( mProgram ); }

	int ComputeShader::Uniform( std::string_view name )
	{
		std::string key( name );
		if ( auto it = mUniformLocations.find( key ); it != mUniformLocations.end() )
			return it->second;
		return mUniformLocations.emplace( key, glGetUniformLocation( mProgram, key.c_str() ) ).first->second;
	}

	void ComputeShader::SetInt( std::string_view name, int value )
	{
		int location = Uniform( name );
		if ( location >= 0 ) glProgramUniform1i( mProgram, location, value );
	}

	void ComputeShader::SetFloat( std::string_view name, float value )
	{
		int location = Uniform( name );
		if ( location >= 0 ) glProgramUniform1f( mProgram, location, value );
	}

	void ComputeShader::SetFloat2( std::string_view name, float x, float y )
	{
		int location = Uniform( name );
		if ( location >= 0 ) glProgramUniform2f( mProgram, location, x, y );
	}

	void ComputeShader::SetInt2( std::string_view name, int x, int y )
	{
		int location = Uniform( name );
		if ( location >= 0 ) glProgramUniform2i( mProgram, location, x, y );
	}

	void ComputeShader::Dispatch( std::uint32_t x, std::uint32_t y, std::uint32_t z ) const
	{
		glUseProgram( mProgram );
		glDispatchCompute( x, y, z );
	}

	void ComputeShader::Barrier()
	{
		glMemoryBarrier( GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT |
			GL_TEXTURE_FETCH_BARRIER_BIT | GL_TEXTURE_UPDATE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT );
	}

} // namespace sl
