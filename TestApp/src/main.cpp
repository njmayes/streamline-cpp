#include "streamline.h"

#include "slc/Common/Reflection.h"
#include "slc/Logging/Targets/ConsoleLogTarget.h"
#include "slc/Types/Enum.h"

#include <iostream>

namespace slc
{
	SLC_MAKE_SMART_ENUM( Error,
						 InvalidChar,
						 ( InvalidRandom, int ));

	SLC_MAKE_SMART_ENUM( Failure, RandomFail );

	using FooResult = Result< int, Error >;
	using BarResult = Result< float, Error >;
	using BazResult = Result< int, Failure >;
	// Demo Types and Functions

	SLC_MAKE_SMART_ENUM( InputError,
						 ( InvalidChar, char ),
						 InvalidState,
						 ( InvalidFormatString, std::string ) )

	using IntInputResult = Result< int, InputError >;
	using StringInputResult = Result< std::string, InputError >;

	using InputData = std::tuple< std::string, int >;

	IntInputResult GetIntegerInput()
	{
		int input;
		if ( !( std::cin >> input ) )
		{
			return Err< IntInputResult >( InputError::InvalidChar );
		}

		return Ok< IntInputResult >( input );
	}

	StringInputResult GetStringInput()
	{
		std::string input;
		if ( !( std::cin >> input ) )
		{
			return Err< StringInputResult >( InputError::InvalidState );
		}

		return Ok< StringInputResult >( input );
	}

	StringInputResult CombineStrings( InputData formatData )
	{
		std::string& fmt = std::get< 0 >( formatData );
		int param = std::get< 1 >( formatData );

		if ( fmt.find( "{0}" ) != std::string::npos )
		{
			return Ok< StringInputResult >( std::vformat( fmt, std::make_format_args( param ) ) );
		}

		return Err< StringInputResult >( InputError::InvalidFormatString );
	}

} // namespace slc

using namespace slc;


struct SerialisationTest
{
	int a, b;

	SLC_JSON_SERIALISE( SerialisationTest, a, b );
};

SLC_MAKE_SMART_ENUM( TestEnum,
					 OutOfBounds,
					 ( Unexpected, std::string ) );

FooResult GetInput()
{
	int input;
	if ( !( std::cin >> input ) )
		return Err< FooResult >( Error::InvalidChar );

	return Ok< FooResult >( input );
}

FooResult GetRandom()
{
	srand( time( NULL ) );
	int val = rand() % 2;

	if ( val == 0 )
		return Ok< FooResult >( 1000 );

	return Err< FooResult >( Error::InvalidRandom, val );
}

FooResult GetRandomTwo( int value )
{
	if ( value % 2 == 0 )
		return Ok< FooResult >( 500 );

	return Err< FooResult >( Error::InvalidRandom, value );
}

BarResult CheckRandom( int val )
{
	if ( val % 2 == 0 )
		return Ok< BarResult >( 3.14159 );

	return Err< BarResult >( Error::InvalidRandom, val );
}

int main( int argc, char* argv[] )
{
	TestEnum test = TestEnum::OutOfBounds;

	test.Match(
		MatchCase< TestEnum::OutOfBounds >( [] { std::cout << "OutOfBounds\n"; } ),
		DefaultCase( []( auto const& value ) { std::cout << "Default case\n"; } ) );


	test = TestEnum( TestEnum::Unexpected, "Unexpected" );

	test.Match(
		MatchCase< TestEnum::OutOfBounds >( [] { std::cout << "OutOfBounds\n"; } ),
		MatchCase< TestEnum::Unexpected >( []( std::string const& value ) { std::cout << std::format( "Unexpected: {}\n", value ); } ) );


	auto a = Do< float >(
		GetRandom(),
		NEXT( CheckRandom ) );

	FooResult b = a.Map< int >( []( float val ) { return ( int )val; } )
					  .MapError< Failure >( []( Error error ) { return Failure::RandomFail; } )
					  .MapError< Error >( []( Failure f ) { return Error::InvalidRandom; } );


	b.Match(
		MatchCase< FooResult::Ok >( []( int value ) { std::cout << "User entered value of " << value << "\n"; } ),
		MatchCase< Error::InvalidChar >( [] { "Invalid character entered\n"; } ),
		DefaultCase( [] { std::cout << "Default case\n"; } ) );

	auto bVal = b.UnwrapOrDefault();

	auto c = Do< float >(
				 GetRandom(),
				 NEXT( CheckRandom ) )
				 .MapOr< std::string >( "Error", []( float val ) { return std::to_string( val ); } );


	auto d = GetRandom() |
			 NEXT( GetRandomTwo ) |
			 CHAIN( GetRandom );


	d.Match(
		MatchCase< FooResult::Ok >( []( int value ) { std::cout << "User entered value of " << value << "\n"; } ),
		MatchCase< Error::InvalidRandom >( []( int value ) { std::cout << "RNG not satisfied\n"; } ),
		DefaultCase( []( auto const& value ) { std::cout << "Default case\n"; } ) );

	auto dVal = d.UnwrapOrElse( []() { return 0; } );
}