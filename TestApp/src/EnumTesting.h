#include "SL/Core.h"

#include <iostream>

namespace sl {

	enum class Error
	{
		InvalidChar,
		InvalidRandom,
	};

	using SmartError = sl::SmartEnum<
		Error,
		sl::Case< Error::InvalidChar >,
		sl::Case< Error::InvalidRandom, int > >;

	enum class Failure
	{
		RandomFail,
	};

	using SmartFailure = sl::SmartEnum<
		Failure,
		sl::Case< Failure::RandomFail > >;

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

	using SmartInputError = sl::SmartEnum<
		InputError,
		sl::Case< InputError::InvalidChar, char >,
		sl::Case< InputError::InvalidState >,
		sl::Case< InputError::InvalidFormatString, std::string > >;

	using IntInputResult = Result< int, InputError >;
	using StringInputResult = Result< std::string, InputError >;

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

	struct SerialisationTest
	{
		int a, b;

		SLC_JSON_SERIALISE( SerialisationTest, a, b );
	};

	inline FooResult GetInput()
	{
		int input;
		if ( !( std::cin >> input ) )
			return Err< FooResult >( SmartError::Make< Error::InvalidChar >() );

		return Ok< FooResult >( input );
	}

	inline FooResult GetRandom()
	{
		srand( time( NULL ) );
		int value = rand() % 5;

		if ( value == 0 )
			return Ok< FooResult >( 1000 );

		return Err< FooResult >( SmartError::Make< Error::InvalidRandom >( value ) );
	}

	inline FooResult GetRandomTwo( int value )
	{
		if ( value % 2 == 0 )
			return Ok< FooResult >( 500 );

		return Err< FooResult >( SmartError::Make< Error::InvalidRandom >( value ) );
	}

	inline BarResult CheckRandom( int value )
	{
		if ( value % 2 == 0 )
			return Ok< BarResult >( 3.14159 );

		return Err< BarResult >( SmartError::Make< Error::InvalidRandom >( value ) );
	}

	enum class ErrorEnum
	{
		OutOfBounds = 4,
		Unexpected,
		Test = Unexpected
	};

	using SmartErrorEnum = SmartEnum<
		ErrorEnum,
		Case< ErrorEnum::Unexpected, std::string >,
		Case< ErrorEnum::OutOfBounds >
		// Case< ErrorEnum::OutOfBounds, std::string >
		>;


	inline void EnumTest()
	{
		auto make_input = [] {
			srand( time( NULL ) );
			int i = rand() % 2;
			if ( i == 0 )
				return SmartErrorEnum::Make< ErrorEnum::Unexpected >( "Actual value" );
			else
				return SmartErrorEnum::Make< ErrorEnum::OutOfBounds >();
		};

		// SmartErrorEnum fooa = SmartErrorEnum::Make< ErrorEnum::Unexpected >( "Actual value" );
		SmartErrorEnum foo = make_input();
		auto bar = foo.Match(
			MatchCase< ErrorEnum::OutOfBounds >( [] { return "OutOfBounds"; } ),
			MatchCase< ErrorEnum::Unexpected >( []( std::string_view value ) { return value; } )
		);

		std::cout << "Value: " << bar << "\n";

		auto a = GetRandom().AndThen( CheckRandom );

		FooResult b = a.Map( []( float val ) { return ( int )val; } )
						  .MapError( []( SmartError error ) -> SmartFailure { return SmartFailure::Make< Failure::RandomFail >(); } )
						  .MapError( []( SmartFailure f ) -> SmartError { return SmartError::Make< Error::InvalidRandom >( 24 ); } );


		b.Match(
			MatchCase< FooResult::Ok >( []( int value ) { std::cout << "User entered value of " << value << "\n"; } ),
			MatchCase< Error::InvalidChar >( [] { std::cout << "Invalid character entered\n"; } ),
			MatchDefault()
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
			MatchDefault( [] { std::cout << "Default case\n"; } )
		);

		auto dVal = d.UnwrapOrElse( []() { return 0; } );
	}

} // namespace sl