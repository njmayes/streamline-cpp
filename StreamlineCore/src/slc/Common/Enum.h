#pragma once

#include "slc/Common/Reflection.h"

#include <bit>
#include <stdexcept>

namespace slc::Enum {

	template < IsEnum T >
	inline static constexpr std::string_view ToString( T enumVal )
	{
		static_assert( MAGIC_ENUM_SUPPORTED, "Compiler does not support magic enums! Define your own conversions!" );

		return magic_enum::enum_name( enumVal );
	}

	template < IsEnum T >
	inline static constexpr std::optional< T > FromString( std::string_view enumStr )
	{
		static_assert( MAGIC_ENUM_SUPPORTED, "Compiler does not support magic enums! Define your own conversions!" );

		auto enumVal = magic_enum::enum_cast< T >( enumStr );
		if ( enumVal.has_value() )
			return enumVal.value();

		return std::nullopt;
	}

	template < IsEnum T >
	inline static constexpr bool Contains( std::underlying_type_t< T > value )
	{
		return magic_enum::enum_contains< T >( value );
	}

	template < IsEnum T >
	inline static constexpr size_t Count()
	{
		return magic_enum::enum_count< T >();
	}

	namespace detail {

		class BitFlagSentinel
		{};

		template < IsEnum T >
		class BitFlagIterator
		{
		private:
			using Underlying = std::underlying_type_t< T >;

		public:
			constexpr BitFlagIterator( Underlying mask )
				: mValue( mask )
			{
				mCurrent = static_cast< Underlying >( mask ? ( 1 << std::countr_zero( mask ) ) : 0 );
				mEnd = static_cast< Underlying >( mask ? std::bit_floor( mask ) : 0 );
			}
			constexpr bool operator!=( BitFlagSentinel const& ) const
			{
				return mCurrent <= mEnd;
			}
			constexpr T operator*() const
			{
				if ( not magic_enum::enum_contains< T >( mCurrent ) )
					throw std::out_of_range( "Enum iterator value invalid!" );

				return static_cast< T >( mCurrent );
			}
			constexpr BitFlagIterator& operator++()
			{
				do
				{
					mCurrent <<= 1;
				} while ( mCurrent <= mEnd and ( mValue & mCurrent ) == 0 );

				return *this;
			}

		private:
			Underlying mValue;
			Underlying mCurrent;
			Underlying mEnd;
		};
	} // namespace detail

	template < IsEnum T >
	class EnumBitMask
	{
	private:
		SCONSTEVAL bool IsEnumAllFlags()
		{
			auto values = magic_enum::enum_values< T >();

			for (auto value : values)
			{
				if ( std::popcount( std::to_underlying( value ) ) > 1 )
					return false;
			}

			return true;
		}

		SASSERT( IsEnumAllFlags(), "EnumBitMask can only be used with enums that are all bit flags" );

	public:
		using Underlying = std::underlying_type_t< T >;

		constexpr EnumBitMask( Underlying mask )
			: mMask( mask )
		{
		}

		template < typename... Bits >
			requires( ( std::same_as< Bits, T > && ... ) )
		constexpr EnumBitMask( Bits... bits )
			: mMask( ( std::to_underlying( bits ) | ... ) )
		{
		}

		constexpr bool Test( T flag ) const
		{
			auto bit = std::to_underlying( flag );
			return ( mMask & bit ) == bit;
		}

		constexpr void Set( T flag )
		{
			mMask = static_cast< T >( std::to_underlying( mMask ) | std::to_underlying( flag ) );
		}

		constexpr auto begin() const
		{
			return detail::BitFlagIterator< T >( mMask );
		}
		constexpr auto end() const
		{
			return detail::BitFlagSentinel{};
		}

	private:
		Underlying mMask;
	};
} // namespace slc::Enum