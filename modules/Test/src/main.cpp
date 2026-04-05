#include "SL/Core.h"
#include "SL/Gfx.h"
#include "SL/Net.h"

#include "CompilationTests.h"

int main( int argc, char** argv )
{
	sl::CommandLineArgs args{ argc, argv };

	sl::test::EnumTest();

	try
	{
		sl::test::ReflectionTestFunc();
	}
	catch ( const std::exception& e )
	{
		std::cerr << "Reflection test failed: " << e.what() << std::endl;
	}
}