#pragma once

#include <slc/Common/Base.h>

namespace slc {

	class UniformBuffer : public RefCounted
	{
	public:
		UniformBuffer(uint32_t size, uint32_t binding);
		virtual ~UniformBuffer();

		void SetData(const void* data, uint32_t size, uint32_t offset = 0);

	private:
		uint32_t mRendererID = 0;
	};
}