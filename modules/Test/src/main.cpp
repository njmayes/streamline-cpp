#include "SL/Core.h"
#include "SL/Gfx.h"
#include "SL/Net.h"

#include "CompilationTests.h"

int main( int argc, char** argv )
{
	sl::CommandLineArgs args{ argc, argv };

	sl::test::EnumTest();

	sl::test::ReflectionTestFunc();
}