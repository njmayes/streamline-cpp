#pragma once


namespace slc {

	template<size_t TSize>
	class StaticBuffer
	{
	public:
		StaticBuffer() = default;
		StaticBuffer(Byte data[TSize])
		{
			memset(mData, 0, TSize);
			memcpy(mData, data, TSize);
		}

		template<size_t _Other>
		StaticBuffer(const StaticBuffer<_Other>& buffer)
		{
			memset(mData, 0, TSize);
			if constexpr (_Other <= TSize)
				memcpy(mData, buffer.mData, _Other);
			else
				memcpy(mData, buffer.mData, TSize);
		}

		Byte& operator[](size_t index)
		{
			return mData[index];
		}

		Byte operator[](size_t index) const
		{
			return mData[index];
		}

		constexpr size_t size() const { return TSize; }

	protected:
		Byte mData[TSize] = { 0 };
	};
}