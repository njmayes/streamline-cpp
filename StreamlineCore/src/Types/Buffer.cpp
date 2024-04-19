#include "Buffer.h"

namespace slc {

	Buffer::Buffer(const Buffer& buffer)
	{
		ASSERT(buffer.mData && buffer.mSize);

		Allocate(buffer.mSize);
		memcpy(mData, buffer.mData, buffer.mSize);
	}

	Buffer::Buffer(Buffer&& buffer) noexcept
	{
		if (&buffer == this)
			return;

		mData = buffer.mData;
		buffer.mData = nullptr;

		mSize = buffer.mSize;
	}

	Buffer& Buffer::operator=(const Buffer& buffer)
	{
		ASSERT(buffer.mData && buffer.mSize);

		Allocate(buffer.mSize);
		memcpy(mData, buffer.mData, buffer.mSize);

		return *this;
	}

	Buffer& Buffer::operator=(Buffer&& buffer) noexcept
	{
		ASSERT(&buffer != this, "Cannot move assign an object to itself!");

		mData = buffer.mData;
		buffer.mData = nullptr;

		mSize = buffer.mSize;

		return *this;
	}

	Buffer Buffer::Copy(const void* data, size_t size)
	{
		return Buffer();
	}

	void Buffer::Allocate(size_t size)
	{
		Release();

		if (size == 0)
			return;

		mData = new Byte[size];
		mSize = size;
	}

	void Buffer::Release()
	{
		delete[](Byte*)mData;
		mData = nullptr;
		mSize = 0;
	}

	Buffer Buffer::CopyBytes(size_t size, size_t offset)
	{
		ASSERT(offset + size <= mSize, "Buffer overflow!");
		return Buffer::Copy((Byte*)mData + offset, size);
	}

	void Buffer::Resize(size_t newSize)
	{
		if (mSize == newSize)
			return;

		Byte* newData = new Byte[newSize];
		memcpy(newData, mData, (newSize > mSize) ? mSize : newSize);
		delete[] mData;

		mData = newData;
		mSize = newSize;
	}
}