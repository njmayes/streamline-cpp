#pragma once

#include "StaticBuffer.h"

namespace slc {

	template < size_t TSize >
	class StaticString : protected StaticBuffer< TSize + 1 >
	{
	public:
		StaticString( std::string_view string )
		{
			auto size = std::min( string.size(), TSize );

			memset( this->mData.data(), 0, TSize + 1 );
			memcpy( this->mData.data(), string.data(), size );
			;
		}

		constexpr size_t Length() const noexcept
		{
			return TSize;
		}

		char* Data() noexcept
		{
			return reinterpret_cast< char* >( this->mData.data() );
		}

		std::string ToString() const
		{
			return reinterpret_cast< const char* >( this->mData.data() );
		}
	};
} // namespace slc