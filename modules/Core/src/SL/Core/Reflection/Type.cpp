#include "Type.h"
#include "Property.h"
#include "Method.h"

namespace sl {

	Property Type::GetProperty( std::string_view name ) const
	{
		auto it = std::ranges::find_if( mInfo->properties, [ name ]( const auto& prop ) { return prop.name == name; } );
		if ( it == mInfo->properties.end() )
			throw std::runtime_error( std::format( "Attempting to get property by name that didn't exist [{}::{}].", mInfo->name, name ) );

		return Property( *it );
	}

	std::vector< Property > Type::GetProperties() const
	{
		return mInfo->properties |
			   std::views::transform( []( const auto& property ) { return Property( property ); } ) |
			   std::ranges::to< std::vector >();
	}

	Method Type::GetMethod( std::string_view name ) const
	{
		auto it = std::ranges::find_if( mInfo->methods, [ name ]( const auto& method ) { return method.name == name; } );
		if ( it == mInfo->methods.end() )
			throw std::runtime_error( std::format( "Attempting to get method by name that didn't exist [{}::{}].", mInfo->name, name ) );

		return Method( *it );
	}

	std::vector< Method > Type::GetMethods() const
	{
		return mInfo->methods |
			   std::views::transform( []( const auto& method ) { return Method( method ); } ) |
			   std::ranges::to< std::vector >();
	}
} // namespace sl