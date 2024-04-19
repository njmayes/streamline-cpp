#pragma once

#include "Common/Base.h"

namespace slc {

	class Buffer
	{
	public:
		Buffer() = default;
		Buffer(std::nullptr_t) {}
		Buffer(void* data, size_t size) : mData(data), mSize(size) {}
		Buffer(size_t size) { Allocate(size); }

		Buffer(const Buffer& buffer);
		Buffer(Buffer&& buffer) noexcept;

		virtual ~Buffer() { Release(); }

		Buffer& operator=(const Buffer& buffer);
		Buffer& operator=(Buffer&& buffer) noexcept;

		static Buffer Copy(const void* data, size_t size);

	public:
		template<IsStandard T>
		T& Read(size_t offset = 0) { return *(T*)((Byte*)mData + offset); }
		template<IsStandard T>
		const T& Read(size_t offset = 0) const { return *(T*)((Byte*)mData + offset); }

		template<typename T>
		T* As() const { return (T*)mData; }

		template<IsStandard T>
		void Set(const T& data, size_t offset = 0)
		{
			constexpr size_t DataSize = sizeof(T);
			if (offset + DataSize > mSize)
				Resize(offset + DataSize);

			memcpy((Byte*)mData + offset, &data, DataSize);
		}

		template<IsStandard T>
		void Pop(T& data)
		{
			constexpr size_t DataSize = sizeof(T);
			data = std::move(Read<T>(mSize - DataSize));
			Resize(mSize - DataSize);
		}

		Byte* Data(size_t offset = 0) { return (Byte*)mData + offset; }
		const Byte* Data(size_t offset = 0) const { return (Byte*)mData + offset; }

		template<IsStandard T>
		Byte* Data(size_t offset = 0) { return (Byte*)mData + (sizeof(T) * offset); }
		template<IsStandard T>
		const Byte* Data(size_t offset = 0) const { return (Byte*)mData + (sizeof(T) * offset); }

		size_t Size() const { return mSize; }
		void Resize(size_t newSize);

		Buffer CopyBytes(size_t size, size_t offset = 0);


	public:
		operator bool() const { return mData != nullptr; }

		Byte& operator[](size_t index) { return ((Byte*)mData)[index]; }
		const Byte& operator[](size_t index) const { return ((Byte*)mData)[index]; }

	protected:
		void Allocate(size_t size);
		void Release();

	protected:
		void* mData = nullptr;
		size_t mSize = 0;
	};
}