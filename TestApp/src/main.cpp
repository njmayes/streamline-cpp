#include "streamline.h"

#include <iostream>

namespace slc {
	enum class Error
	{
		InvalidChar,
		InvalidRandom,
	};
	using SmartError = slc::SmartEnum<
		Error,
		slc::Case< Error::InvalidChar >,
		slc::Case< Error::InvalidRandom, int > >;

	enum class Failure
	{
		RandomFail,
	};
	using SmartFailure = slc::SmartEnum<
		Failure,
		slc::Case< Failure::RandomFail > >;

	using FooResult = Result< int, SmartError >;
	using BarResult = Result< float, SmartError >;
	using BazResult = Result< int, SmartFailure >;
	// Demo Types and Functions

	enum class InputError
	{
		InvalidChar,
		InvalidState,
		InvalidFormatString,
	};

	using SmartInputError = slc::SmartEnum<
		InputError,
		slc::Case< InputError::InvalidChar, char >,
		slc::Case< InputError::InvalidState >,
		slc::Case< InputError::InvalidFormatString, std::string > >;

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

FooResult GetInput()
{
	int input;
	if ( !( std::cin >> input ) )
		return Err< FooResult >( SmartError::Make< Error::InvalidChar >() );

	return Ok< FooResult >( input );
}

FooResult GetRandom()
{
	srand( time( NULL ) );
	int value = rand() % 5;

	if ( value == 0 )
		return Ok< FooResult >( 1000 );

	return Err< FooResult >( SmartError::Make< Error::InvalidRandom >( value ) );
}

FooResult GetRandomTwo( int value )
{
	if ( value % 2 == 0 )
		return Ok< FooResult >( 500 );

	return Err< FooResult >( SmartError::Make< Error::InvalidRandom >( value ) );
}

BarResult CheckRandom( int value )
{
	if ( value % 2 == 0 )
		return Ok< BarResult >( 3.14159 );

	return Err< BarResult >( SmartError::Make< Error::InvalidRandom >( value ) );
}

enum class ErrorEnum
{
	OutOfBounds,
	Unexpected
};

using SmartErrorEnum = slc::SmartEnum<
	ErrorEnum,
	slc::Case< ErrorEnum::OutOfBounds >,
	slc::Case< ErrorEnum::Unexpected, std::string > >;


int main( int argc, char* argv[] )
{
	SmartErrorEnum asasfa = SmartErrorEnum::Make< ErrorEnum::Unexpected >( "Actual value" );
	auto test = asasfa.Match(
		slc::MatchCase< ErrorEnum::OutOfBounds >( [] { std::cout << "OutOfBounds\n"; return 2; } ),
		slc::MatchCase< ErrorEnum::Unexpected >( []( std::string const& value ) { std::cout << "Unexpected: " << value << "\n"; return 2; } )
	);


	auto result = test;

	// ErrorEnum test = ErrorEnum::OutOfBounds;

	//// test.Match(
	////	MatchCase< TestEnum::OutOfBounds >( [] { std::cout << "OutOfBounds\n"; } ),
	////	DefaultCase( [] { std::cout << "Default case\n"; } )
	////);


	////test = ErrorEnum( ErrorEnum::Unexpected, "Unexpected" );

	// test.Match(
	//	MatchCase< ErrorEnum::OutOfBounds >( [] { std::cout << "OutOfBounds\n"; } ),
	//	MatchCase< ErrorEnum::Unexpected >( []( std::string const& value ) { std::cout << std::format( "Unexpected: {}\n", value ); } )
	//);


	auto a = GetRandom().AndThen( CheckRandom );

	FooResult b = a.Map( []( float val ) { return ( int )val; } )
					  .MapError( []( SmartError error ) -> SmartFailure { return SmartFailure::Make< Failure::RandomFail >(); } )
					  .MapError( []( SmartFailure f ) -> SmartError { return SmartError::Make< Error::InvalidRandom >( 24 ); } );


	b.Match(
		MatchCase< FooResult::Ok >( []( int value ) { std::cout << "User entered value of " << value << "\n"; } ),
		MatchCase< Error::InvalidChar >( [] { std::cout << "Invalid character entered\n"; } ),
		MatchCase< Error::InvalidRandom >( [] ( int value ) { std::cout << "Invalid random value\n"; } ),
		DefaultCase( [] { std::cout << "Default case\n"; } )
	);

	 auto bVal = b.UnwrapOrDefault();

	 auto c = GetRandom()
				 .AndThen( CheckRandom )
				 .MapOr( "Error", []( float val ) { return std::to_string( val ); } );


	 auto d = GetRandom()
				 .AndThen( GetRandomTwo )
				 .OrElse( GetRandom );


	 d.Match(
		MatchCase< FooResult::Ok >( []( int value ) { std::cout << "User entered value of " << value << "\n"; } ),
		MatchCase< Error::InvalidRandom >( []( int value ) { std::cout << "RNG not satisfied\n"; } ),
		DefaultCase( [] { std::cout << "Default case\n"; } )
	);

	 auto dVal = d.UnwrapOrElse( []() { return 0; } );
}