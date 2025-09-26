#include "streamline.h"

#include "ChatRoom.h"
#include "ChatClient.h"

#include "slc/Common/EntryPoint.h"

#include <iostream>
#include <csignal>

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

#define TEST( y ) int y = 0;
SLC_FOR_EACH( TEST, x )


slc::Application* NetServerTest( int argc, char* argv[] )
{
	if ( argc < 2 )
	{
		return nullptr;
	}

	auto spec = slc::MakeBox< slc::ApplicationSpecification >();
	spec->headless = true;
	spec->workingDir = "C:/Users/NMayes/Desktop/Reference/streamline-cpp/TestApp";

	slc::net::ServerContextOptions opts{};
	opts.num_threads = 5;
	opts.cert_file = "server.crt";
	opts.key_file = "server.key";

	opts.ports = std::vector< std::uint16_t >( argc - 1 );

	for ( int i = 1; i < argc; ++i )
	{
		unsigned short port = std::atoi( argv[ i ] );
		opts.ports[ i - 1 ] = port;
	}

	return new ChatServer( std::move( spec ), opts );
}

slc::Application* NetClientTest( int argc, char* argv[] )
{
	if ( argc < 2 )
	{
		return nullptr;
	}

	auto spec = slc::MakeBox< slc::ApplicationSpecification >();
	spec->workingDir = "C:/Users/NMayes/Desktop/Reference/streamline-cpp/TestApp";

	slc::net::ClientContextOptions opts{};

	const char* host = argv[ 1 ];
	unsigned short port = std::atoi( argv[ 2 ] );

	opts.host = host;
	opts.port = port;

	return new ChatClient( std::move( spec ), opts );
}

slc::Application* CreateApplication( int argc, char** argv )
{
	if ( std::string( argv[ 1 ] ) == "server" )
		return NetServerTest( argc - 1, &argv[ 1 ] );
	else if ( std::string( argv[ 1 ] ) == "client" )
		return NetClientTest( argc - 1, &argv[ 1 ] );
}

// static auto constexpr Filename = "measurements.txt";
// static auto constexpr RepeatCount = 10;
//
// template < typename T >
// void RunChallenge( bool print = false )
//{
//	Timer timer;
//
//	for ( auto i = 0; i < RepeatCount; i++ )
//	{
//		T brc{ Filename };
//		brc.Run();
//
//		if ( print )
//			brc.Print();
//	}
//
//	log::Info( "Average time for completion ({}): {}s", TypeTraits< T >::Name, timer.Elapsed() / RepeatCount );
// }
//
// template < typename T >
// void RunChallengeTests()
//{
//	auto chunk_size = 1_MB;
//
//	for ( auto i = 0; i < 8; i++ )
//	{
//		Timer timer;
//
//		for ( auto i = 0; i < RepeatCount; i++ )
//		{
//			T brc{ Filename, chunk_size };
//			brc.Run();
//		}
//
//		log::Info( "Average time for completion ({}MB Chunks): {}s", chunk_size / ( 1024 * 1024 ), timer.Elapsed() / RepeatCount );
//
//		chunk_size *= 2;
//	}
// }
//
// int main( int argc, char* argv[] )
//{
//
//	slc::Logger::GetGlobalLogger().AddLogTarget< slc::ConsoleLogTarget >( slc::LogLevel::Info );
//
//	RunChallengeTests< v2::BillionRows >();
//
//	// RunChallenge< v1::BillionRows >();
//	// RunChallenge< v2::BillionRows >();
//	// RunChallenge< v3::BillionRows >();
//	// RunChallenge< v4::BillionRows >();
//	// RunChallenge< v5::BillionRows >();
//	// RunChallenge< v6::BillionRows >();
//
//	if ( argc < 2 )
//		return -1;
//
//	if ( std::string( argv[ 1 ] ) == "server" )
//		NetServerTest( argc - 1, &argv[ 1 ] );
//	else if ( std::string( argv[ 1 ] ) == "client" )
//		NetClientTest( argc - 1, &argv[ 1 ] );
//
//	 auto make_input = [] {
//		srand( time( NULL ) );
//		int i = rand() % 2;
//		if ( i == 0 )
//			return SmartErrorEnum::Make< ErrorEnum::Unexpected >( "Actual value" );
//		else
//			return SmartErrorEnum::Make< ErrorEnum::OutOfBounds >();
//	 };
//
//	//SmartErrorEnum fooa = SmartErrorEnum::Make< ErrorEnum::Unexpected >( "Actual value" );
//	 SmartErrorEnum foo = make_input();
//	 auto bar = foo.Match(
//		MatchCase< ErrorEnum::OutOfBounds >( [] { return "OutOfBounds"; } ),
//		MatchCase< ErrorEnum::Unexpected >( []( std::string_view value ) { return value; } )
//	);
//
//	 std::cout << "Value: " << bar << "\n";
//
//	 auto a = GetRandom().AndThen( CheckRandom );
//
//	 FooResult b = a.Map( []( float val ) { return ( int )val; } )
//					  .MapError( []( SmartError error ) -> SmartFailure { return SmartFailure::Make< Failure::RandomFail >(); } )
//					  .MapError( []( SmartFailure f ) -> SmartError { return SmartError::Make< Error::InvalidRandom >( 24 ); } );
//
//
//	 b.Match(
//		MatchCase< FooResult::Ok >( []( int value ) { std::cout << "User entered value of " << value << "\n"; } ),
//		MatchCase< Error::InvalidChar >( [] { std::cout << "Invalid character entered\n"; } ),
//		MatchDefault()
//	);
//
//	 auto bVal = b.UnwrapOrDefault();
//
//	 auto c = GetRandom()
//				 .AndThen( CheckRandom )
//				 .MapOr( "Error", []( float val ) { return std::to_string( val ); } );
//
//
//	 auto d = GetRandom()
//				 .AndThen( GetRandomTwo )
//				 .OrElse( GetRandom );
//
//
//	 d.Match(
//		MatchCase< FooResult::Ok >( []( int value ) { std::cout << "User entered value of " << value << "\n"; } ),
//		MatchCase< Error::InvalidRandom >( []( int value ) { std::cout << "RNG not satisfied\n"; } ),
//		MatchDefault( [] { std::cout << "Default case\n"; } )
//	);
//
//	 auto dVal = d.UnwrapOrElse( []() { return 0; } );
// }