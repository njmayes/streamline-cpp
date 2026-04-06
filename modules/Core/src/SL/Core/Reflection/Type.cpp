#include "Type.h"
#include "Property.h"
#include "Method.h"

namespace sl {

	std::string Type::GetName() const
	{
		return reflect::Reflection::TypeRefToString( mInfo );
	}

	Property Type::GetProperty( std::string_view name ) const
	{
		auto it = std::ranges::find_if( mInfo.base->properties, [ name ]( const auto& prop ) { return prop.name == name; } );
		if ( it == mInfo.base->properties.end() )
			throw std::runtime_error( std::format( "Attempting to get property by name that didn't exist [{}::{}].", mInfo.base->name, name ) );

		return Property( *it );
	}

	std::vector< Property > Type::GetProperties() const
	{
		return mInfo.base->properties |
			   std::views::transform( []( const auto& property ) { return Property( property ); } ) |
			   std::ranges::to< std::vector >();
	}

	Method Type::GetMethod( std::string_view name ) const
	{
		auto it = std::ranges::find_if( mInfo.base->methods, [ name ]( const auto& method ) { return method.name == name; } );
		if ( it == mInfo.base->methods.end() )
			throw std::runtime_error( std::format( "Attempting to get method by name that didn't exist [{}::{}].", mInfo.base->name, name ) );

		return Method( *it );
	}

	std::vector< Method > Type::GetMethods() const
	{
		return mInfo.base->methods |
			   std::views::transform( []( const auto& method ) { return Method( method ); } ) |
			   std::ranges::to< std::vector >();
	}

	std::string Type::ToString() const
	{
		std::stringstream ss;

		ss << std::format( "Type: {}\n", GetName() );
		ss << "Properties:\n";

		auto const& props = GetProperties();

		auto member_name_sizes = props | std::views::transform( &Property::GetName ) | std::views::transform( &std::string_view::length ) ;
		std::size_t max_name_length = std::ranges::max( member_name_sizes );

		// 2. Print aligned
		for ( auto const& member : props )
		{
			ss << std::format(
				"\t- {:<{}} [{}]\n",
				member.GetName(),
				max_name_length,
				member.GetType().GetName()
			);
		}

		ss << "Methods:\n";
		for ( auto const& member : GetMethods() )
		{
			ss << std::format( "\t- {}\n", member.GetName() );
		}

		auto result = ss.str();
		result.pop_back(); // remove last newline
		return result;
	}


	std::string Type::ToString( Instance obj ) const
	{
		std::stringstream ss;
		ss << std::format( "Type: {}\n", GetName() );
		ss << "Properties:\n";

		auto const& props = GetProperties();

		auto member_names = props | std::views::transform( &Property::GetName );
		auto member_name_sizes = member_names | std::views::transform( &std::string_view::length );

		std::size_t max_name_length = std::ranges::max( member_name_sizes );

		auto member_values = props | std::views::transform( [ &obj ]( const auto& member ) { return std::format( "{}", member.GetValue( obj ) ); } );
		auto member_value_sizes = member_values | std::views::transform( &std::string::length );

		std::size_t max_value_length = std::ranges::max( member_value_sizes );

		for ( auto const& [ name, value, member ] : std::views::zip( member_names, member_values, props ) )
		{
			ss << std::format( "\t- {:<{}} = {:<{}} [{}]\n", name, max_name_length, value, max_value_length, member.GetType().GetName() );
		}

		ss << "Methods:\n";
		for ( auto const& member : GetMethods() )
		{
			ss << std::format( "\t- {}\n", member.GetName() );
		}

		auto result = ss.str();
		result.pop_back(); // remove last newline
		return result;
	}
} // namespace sl