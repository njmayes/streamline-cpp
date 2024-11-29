#pragma once

#include <array>

namespace slc {

	template<size_t TSize>
	class StaticBuffer
	{
	public:
		StaticBuffer() = default;
		StaticBuffer(const Byte(&data)[TSize])
		{
			memset(mData.data(), 0, TSize);
			memcpy(mData.data(), data, TSize);
		}

		template<size_t TOther>
		StaticBuffer(const StaticBuffer<TOther>& buffer)
		{
			memset(mData.data(), 0, TSize);

			constexpr auto Size = std::min(TOther, TSize);
			memcpy(mData.data(), buffer.mData.data(), Size);
		}

		Byte& operator[](size_t index)
		{
			return mData[index];
		}

		Byte operator[](size_t index) const
		{
			return mData[index];
		}

		constexpr size_t Size() const noexcept { return TSize; }

	protected:
		std::array<Byte, TSize> mData;
	};
}