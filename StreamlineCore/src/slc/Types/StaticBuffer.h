#pragma once

#include <array>

namespace slc {

	template<size_t TSize>
	class StaticBuffer
	{
	public:
		StaticBuffer() = default;
		StaticBuffer(Byte data[TSize])
		{
			memset(mData.data(), 0, TSize);
			memcpy(mData.data(), data, TSize);
		}

		template<size_t _Other>
		StaticBuffer(const StaticBuffer<_Other>& buffer)
		{
			memset(mData.data(), 0, TSize);
			if constexpr (_Other <= TSize)
				memcpy(mData.data(), buffer.mData.data(), _Other);
			else
				memcpy(mData.data(), buffer.mData.data(), TSize);
		}

		Byte& operator[](size_t index)
		{
			return mData[index];
		}

		Byte operator[](size_t index) const
		{
			return mData[index];
		}

		constexpr size_t Size() const { return TSize; }

	protected:
		std::array<Byte, TSize> mData;
	};
}