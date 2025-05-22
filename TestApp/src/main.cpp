#include "streamline.h"

#include "slc/Common/Reflection.h"
#include "slc/Logging/Targets/ConsoleLogTarget.h"
#include "slc/Types/Enum.h"

#include <iostream>

namespace slc
{

	SLC_MAKE_RUST_ENUM( Error,
						InvalidChar,
						( InvalidRandom, int ), );

	SLC_MAKE_RUST_ENUM( Failure, RandomFail );

	using FooResult = Result< int, Error >;
	using BarResult = Result< float, Error >;
	using BazResult = Result< int, Failure >;

	class TestLayer : public ApplicationLayer
	{
	private:
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

	public:
		virtual void OnAttach() override
		{}
		virtual void OnDetach() override
		{}
		virtual void OnUpdate( Timestep ts ) override
		{
			auto a = Do< float >(
				GetRandom(),
				NEXT( CheckRandom ) );

			FooResult b = a.map< int >( []( float val ) { return ( int )val; } )
							  .map_err< Failure >( []( Error error ) { return Failure::RandomFail; } )
							  .map_err< Error >( []( Failure f ) { return Error::InvalidRandom; } );


			b.match(
				MatchCase< FooResult::Ok >( []( int value ) { std::cout << "User entered value of " << value << "\n"; } ),
				MatchCase< Error::InvalidChar >( [] { "Invalid character entered\n"; } ),
				MatchCase< Error::InvalidRandom >( []( int value ) { std::cout << "RNG not satisfied\n"; } ) 
			);

			auto bVal = b.unwrap_or_default();

			auto c = Do< float >(
						 GetRandom(),
						 NEXT( CheckRandom ) )
						 .map_or< std::string >( "Error", []( float val ) { return std::to_string( val ); } );


			auto d = GetRandom() |
					 NEXT( GetRandomTwo ) |
					 CHAIN( GetRandom );

			
			d.match(
				MatchCase< FooResult::Ok >( []( int value ) { std::cout << "User entered value of " << value << "\n"; } ),
				MatchCase< Error::InvalidChar >( [] { "Invalid character entered\n"; } ),
				MatchCase< Error::InvalidRandom >( []( int value ) { std::cout << "RNG not satisfied\n"; } ) 
			);

			auto dVal = d.unwrap_or_else( []() { return 0; } );
		}
		virtual void OnRender() override
		{}
		virtual void OnOverlayRender() override
		{}
		virtual void OnEvent( Event& e ) override
		{}

		LISTENING_EVENTS( EventType::KeyPressed, EventType::MouseButtonPressed )
	};

	class TestApp : public Application
	{
	public:
		TestApp( Impl< ApplicationSpecification > spec )
			: Application( std::move( spec ) )
		{
			PushLayer< TestLayer >();

			AddLogTarget< ConsoleLogTarget >( LogLevel::Debug );
		}

		~TestApp()
		{
		}
	};


	// Demo Types and Functions

	SLC_MAKE_RUST_ENUM( InputError,
						( InvalidChar, char ),
						InvalidState,
						( InvalidFormatString, std::string ) )

	using IntInputResult = Result< int, InputError >;
	using StringInputResult = Result< std::string, InputError >;

	using InputData = std::tuple< std::string, int >;

	IntInputResult GetIntegerInput();
	StringInputResult GetStringInput();
	StringInputResult CombineStrings( InputData formatData );

	// Functional Demo

	// int main()
	//{
	//	using Test = TypeTraits<int>;
	//	bool isIn = Test::InTypes<bool, long, int>;

	//	// Pattern matching
	//	auto intInput = GetIntegerInput();
	//	MATCH_START(intInput)
	//		MATCH_OK(std::cout << "User entered value of " << value << ".\n")
	//		MATCH(InputError::InvalidChar, std::cout << "Entered string was not an integer.\n")
	//		MATCH(InputError::InvalidState, std::cout << "Entered string was invalid.\n")
	//	MATCH_END;

	//	auto strInput = GetStringInput();
	//	MATCH_START(strInput)
	//		MATCH_OK(std::cout << "User entered value of " << value << ".\n")
	//		MATCH(InputError::InvalidChar, std::cout << "Entered string contained an invalid char.\n")
	//		MATCH(InputError::InvalidState, std::cout << "Entered string was invalid.\n")
	//	MATCH_END;

	//	// Do notation
	//	auto finalStringResult = Do<std::string>
	//	(
	//		strInput.and_then<InputData>([&](std::string&& strVal) { return intInput.map<InputData>([&](int intVal) { return std::make_tuple(strVal, intVal); }); }),
	//		NEXT(CombineStrings)
	//	);

	//	std::cout << finalStringResult.unwrap_or("Hello World!") << std::endl;

	//	return 0;
	//}

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


	using namespace std::chrono_literals;

	Task< int > TaskTestAsync()
	{
		std::cout << "TaskTestAsync\n";
		co_return 0;
	}

	Task< int > LazyTaskTestAsync()
	{
		std::cout << "LazyTaskTestAsync\n";
		co_return 0;
	}
	Task< int > TaskTestAsyncInt()
	{
		std::cout << "TaskTestAsyncInt\n";
		auto taskI = LazyTaskTestAsync();
		auto taskH = TaskTestAsync();

		int h = co_await taskH;
		int i = co_await taskI;

		co_return i + h;
	}

} // namespace slc

using namespace slc;


struct SerialisationTest
{
	int a, b;

	SLC_JSON_SERIALISE( SerialisationTest, a, b );
};

Application* CreateApplication( int argc, char** argv )
{
	Impl< ApplicationSpecification > spec = MakeImpl< ApplicationSpecification >();
	spec->name = "TestApp";

	return new TestApp( std::move( spec ) );
}

SLC_MAKE_RUST_ENUM( TestEnum,
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
		MatchCase< TestEnum::Unexpected >( []( std::string const& value ) { std::cout << std::format( "Unexpected: {}\n", value ); } ) 
	);

	test = TestEnum::Unexpected;

	test.Match(
		MatchCase< TestEnum::OutOfBounds >( [] { std::cout << "OutOfBounds\n"; } ),
		MatchCase< TestEnum::Unexpected >( []( std::string const& value ) { std::cout << std::format( "Unexpected: {}\n", value ); } )
	);


	
	auto a = Do< float >(
		GetRandom(),
		NEXT( CheckRandom ) );

	FooResult b = a.map< int >( []( float val ) { return ( int )val; } )
						.map_err< Failure >( []( Error error ) { return Failure::RandomFail; } )
						.map_err< Error >( []( Failure f ) { return Error::InvalidRandom; } );


	b.match(
		MatchCase< FooResult::Ok >( []( int value ) { std::cout << "User entered value of " << value << "\n"; } ),
		MatchCase< Error::InvalidChar >( [] { "Invalid character entered\n"; } ),
		MatchCase< Error::InvalidRandom >( []( int value ) { std::cout << "RNG not satisfied\n"; } ) 
	);

	auto bVal = b.unwrap_or_default();

	auto c = Do< float >(
					GetRandom(),
					NEXT( CheckRandom ) 
			).map_or< std::string >( "Error", []( float val ) { return std::to_string( val ); } );


	auto d = GetRandom() |
				NEXT( GetRandomTwo ) |
				CHAIN( GetRandom );

			
	d.match(
		MatchCase< FooResult::Ok >( []( int value ) { std::cout << "User entered value of " << value << "\n"; } ),
		MatchCase< Error::InvalidChar >( [] { "Invalid character entered\n"; } ),
		MatchCase< Error::InvalidRandom >( []( int value ) { std::cout << "RNG not satisfied\n"; } ) 
	);

	auto dVal = d.unwrap_or_else( []() { return 0; } );
}