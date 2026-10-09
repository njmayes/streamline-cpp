#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>

namespace sl {

	// OpenGL 4.5 compute program. All methods require a current GL context.
	class ComputeShader
	{
	public:
		ComputeShader( std::string_view name, std::string_view source );
		~ComputeShader();

		ComputeShader( ComputeShader const& ) = delete;
		ComputeShader& operator=( ComputeShader const& ) = delete;

		bool IsValid() const { return mProgram != 0; }
		std::string const& GetError() const { return mError; }

		void Bind() const;
		void SetInt( std::string_view name, int value );
		void SetFloat( std::string_view name, float value );
		void SetFloat2( std::string_view name, float x, float y );
		void SetInt2( std::string_view name, int x, int y );
		void Dispatch( std::uint32_t x, std::uint32_t y = 1, std::uint32_t z = 1 ) const;

		// Synchronizes image and SSBO writes between compute passes and texture sampling.
		static void Barrier();
		static bool IsSupported();

	private:
		int Uniform( std::string_view name );
		std::uint32_t mProgram = 0;
		std::string mError;
		std::unordered_map< std::string, int > mUniformLocations;
	};

} // namespace sl
