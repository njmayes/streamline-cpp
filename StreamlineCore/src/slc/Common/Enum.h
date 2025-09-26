#pragma once

#include "slc/Common/Reflection.h"

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
} // namespace slc::Enum