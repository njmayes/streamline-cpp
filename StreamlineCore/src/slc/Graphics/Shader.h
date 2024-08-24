#pragma once

#include <slc/Common/Base.h>
#include <slc/Types/Math.h>

typedef unsigned int GLenum;

namespace slc {

	class Shader : public RefCounted
	{
	public:
		Shader(std::string_view filepath);
		Shader(std::string_view name, std::string_view vertexSrc, std::string_view fragmentSrc);
		~Shader();

		void Bind() const;
		void Unbind() const;

		std::string_view GetName() const { return mName; }

		void SetInt(std::string_view name, int value);
		void SetIntArray(std::string_view name, int* values, uint32_t count);
		void SetMat4(std::string_view name, const Matrix4& value);
		void SetFloat(std::string_view name, float value);
		void SetFloat3(std::string_view name, const Vector3& value);
		void SetFloat4(std::string_view name, const Vector4& value);

	private:
		int GetUniformLocation(std::string_view name) const;

		std::unordered_map<GLenum, std::string> PreProcess(std::string_view source);
		void CompileOrGetVulkanBinaries(const std::unordered_map<GLenum, std::string>& shaderSources);
		void CompileOrGetOpenGLBinaries();
		void CreateProgram();
		void Reflect(GLenum stage, const std::vector<uint32_t>& shaderData);

	private:
		uint32_t mRendererID;

		std::string mFilepath;
		std::string mName;

		std::unordered_map<GLenum, std::vector<uint32_t>> mVulkanSPIRV;
		std::unordered_map<GLenum, std::vector<uint32_t>> mOpenGLSPIRV;

		std::unordered_map<GLenum, std::string> mOpenGLSourceCode;

		mutable std::unordered_map<std::string_view, int> mUniformLocCache;
	};
}