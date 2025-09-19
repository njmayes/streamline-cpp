#pragma once

#include "StaticBuffer.h"

namespace slc {

	template < size_t TSize >
	class StaticString : public StaticBuffer< TSize + 1 >
	{
	public:
		StaticString( std::string_view string )
		{
			auto size = std::min( string.size(), TSize );

			memset( this->mData.data(), 0, TSize + 1 );
			memcpy( this->mData.data(), string.data(), size );
		}

		constexpr size_t Length() const noexcept
		{
			return TSize;
		}

		std::string ToString() const
		{
			return reinterpret_cast< const char* >( this->mData.data() );
		}

		std::string_view ToView() const
		{
			return std::string_view( this->Data(), Length() );
		}
	};
} // namespace slc