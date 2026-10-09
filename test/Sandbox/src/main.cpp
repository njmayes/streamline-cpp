#include "CompilationTests.h"

int main( int argc, char** argv )
{
	sl::CommandLineArgs args{ argc, argv };

	sl::FunctionalTests::Test();

	sl::ReflectionTests::Test();

	sl::AsyncTests::Test();
}