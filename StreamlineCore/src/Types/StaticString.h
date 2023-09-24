#pragma once

#include "StaticBuffer.h"

namespace slc {

	template<size_t TSize>
	class StaticString : public StaticBuffer<TSize>
	{
	public:
		StaticString(const std::string& string)
		{
			ASSERT(string.size() <= TSize);

			memset(this->mData, 0, TSize);
			memcpy(this->mData, string.c_str(), string.size());;
		}

		constexpr size_t length() const { return TSize; }

		operator char* ()
		{
			ASSERT(this->mData[TSize - 1] == 0); // At least the last character should be null
			return (char*)this->mData;
		}

		std::string toString() const
		{
			ASSERT(this->mData[TSize - 1] == 0); // At least the last character should be null
			return (const char*)this->mData;
		}
	};
}