#include "CompilationTests.h"

int main( int argc, char** argv )
{
	sl::CommandLineArgs args{ argc, argv };
	sl::log::AddGlobalLogTarget< sl::ConsoleLogTarget >( sl::LogLevel::Info );

	sl::FunctionalTests::Test();

	sl::ReflectionTests::Test();

	sl::AsyncTests::Test();
}