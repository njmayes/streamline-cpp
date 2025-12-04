#pragma once

#include <array>

namespace sl {

	template < size_t TSize >
	class StaticBuffer
	{
	public:
		StaticBuffer()
		{
			memset( mData.data(), 0, TSize );
		}
		StaticBuffer( const Byte ( &data )[ TSize ] )
		{
			memset( mData.data(), 0, TSize );
			memcpy( mData.data(), data, TSize );
		}

		template < size_t TOther >
		StaticBuffer( const StaticBuffer< TOther >& buffer )
		{
			memset( mData.data(), 0, TSize );

			constexpr auto Size = std::min( TOther, TSize );
			memcpy( mData.data(), buffer.mData.data(), Size );
		}

		auto operator<=>( StaticBuffer const& ) const = default;

		Byte& operator[]( size_t index )
		{
			return mData[ index ];
		}

		Byte operator[]( size_t index ) const
		{
			return mData[ index ];
		}

		char* Data() noexcept
		{
			return reinterpret_cast< char* >( this->mData.data() );
		}

		const char* Data() const noexcept
		{
			return reinterpret_cast< const char* >( this->mData.data() );
		}

		constexpr size_t Size() const noexcept
		{
			return TSize;
		}

		struct Hasher
		{
			std::size_t operator()( StaticBuffer< TSize > const& s ) const noexcept
			{
				auto* data = reinterpret_cast< const char* >( s.mData.data() );
				std::string_view sv{ data, TSize };
				return std::hash< std::string_view >{}( sv );
			}
		};

	protected:
		std::array< Byte, TSize > mData;
	};
} // namespace sl
