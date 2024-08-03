#pragma once

#include "Buffer.h"

namespace slc {

	class VertexArray : public RefCounted
	{
	public:
		VertexArray();
		virtual ~VertexArray();

		virtual void Bind() const;
		virtual void Unbind() const;

		virtual void AddVertexBuffer(const Ref<VertexBuffer>& vertexBuffer);
		virtual void SetIndexBuffer(const Ref<IndexBuffer>& indexBuffer);

		virtual const std::vector<Ref<VertexBuffer>>& GetVertexBuffers() const { return mVertexBuffers; }
		virtual const Ref<IndexBuffer>& GetIndexBuffer() const { return mIndexBuffer; }

	private:
		uint32_t mRendererID;
		uint32_t mVertexBufferIndex = 0;
		std::vector<Ref<VertexBuffer>> mVertexBuffers;
		Ref<IndexBuffer> mIndexBuffer;

	};

}