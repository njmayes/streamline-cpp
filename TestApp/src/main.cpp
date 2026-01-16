#include "SL/Core.h"

#include "ChatRoom.h"
#include "ChatClient.h"

sl::Application* NetServerTest( sl::CommandLineArgs const& args )
{
	using namespace sl::net;

	auto spec = sl::Ref< sl::ApplicationSpecification >::Create();

	auto opts = sl::Read< ServerContextOptions >(
		args,
		sl::Field< ServerContextOptions >( "num_threads", 'n', &ServerContextOptions::num_threads, 1 ),
		sl::Field< ServerContextOptions >( "cert", 'c', &ServerContextOptions::cert_file, "server.crt" ),
		sl::Field< ServerContextOptions >( "key", 'k', &ServerContextOptions::key_file, "server.key" ),
		sl::Field< ServerContextOptions >( "ports", 'p', &ServerContextOptions::ports, std::vector< std::uint16_t >{} )
	);

	if ( not opts.has_value() )
		throw std::runtime_error( "Failed to parse server options from command line" );

	return new ChatServer( spec, *opts );
}

sl::Application* NetClientTest( sl::CommandLineArgs const& args )
{
	using namespace sl::net;

	auto spec = sl::Ref< sl::GuiApplicationSpecification >::Create();

	auto opts = sl::Read< ClientContextOptions >(
		args,
		sl::Field< ClientContextOptions >( "num_threads", 'n', &ClientContextOptions::num_threads, 1 ),
		sl::Field< ClientContextOptions >( "host", 'h', &ClientContextOptions::host ),
		sl::Field< ClientContextOptions >( "port", 'p', &ClientContextOptions::port )
	);

	if ( not opts.has_value() )
		throw std::runtime_error( "Failed to parse client options from command line" );

	return new ChatClient( spec, *opts );
}

sl::Application* CreateApplication( sl::CommandLineArgs args )
{
	if ( args.Empty() )
		return nullptr;

	auto app_type = args.PopFront();

	if ( app_type == "server" )
		return NetServerTest( args );
	else if ( app_type == "client" )
		return NetClientTest( args );

	return nullptr;
}

int main( int argc, char** argv )
{
	sl::CommandLineArgs args{ argc, argv };
	sl::Application::Run( CreateApplication, args );
}