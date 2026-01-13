# streamline-cpp

This is a general purpose C++23 framework/library that contains a collection of shared code from various projects I have made over the years.

The basis of the library uses a mixture of code from my WIP game engine [Labyrinth Engine](https://github.com/amayesingnathan/LabyrinthEngine) 
as well as my C++20 Dear ImGui wrapper [imgui-cpp](https://github.com/amayesingnathan/imgui-cpp).

This is combined with various other common bits of code and ideas, such as Rust-style Enums and Result types complete with a form of basic
pattern matching and Do notation, or a runtime reflection system, provides a selection of tools to save time rewriting application boilerplate code 
and reusing code ideas in different places.

The framework consists of two modules, a core module that provides the basic application framework as well as other utilities, 
The framework consists of two modules, a gfx) module that provides the basic application framework as well as other utilities, 
and an optional graphics module that provides a minimal 2D renderer API that uses OpenGL as well as Dear ImGui for simple interfaces.

## Installation

The framework currently only supports CMake, and is most easily consumed using `FetchContent`.

Two CMake options are provided:

* STREAMLINE_BUILD_GFX - Builds the optional graphics library
* STREAMLINE_BUILD_TEST_APP - Bulds the test app which contains some sample code.

Both options are off by default.

### Example

CMakeLists.txt
```cmake
set(STREAMLINE_BUILD_GFX ON CACHE BOOL "Build gfx lib" FORCE)
set(STREAMLINE_BUILD_TEST_APP OFF CACHE BOOL "Don't build test app" FORCE)

FetchContent_Declare(
    streamline
    GIT_REPOSITORY https://github.com/njmayes/streamline-cpp/
    GIT_TAG        main
)

FetchContent_MakeAvailable(streamline)

target_link_libraries(your_application PRIVATE 
    streamline::core
    streamline::gfx
)
```

## Usage

The framework itself can be consumed as a basic set of utilities included for use in your program, or it can be run as an application directly by doing the following in your main function.

```cpp
#include <streamline-core.h>

sl::Application* CreateApplication( sl::CommandLineArgs const& args );

int main( int argc, char* argv[] )
{
	sl::CommandLineArgs args{ argc, argv };
	sl::Application::Run( CreateApplication, args );
}
```

The `streamline-core.h` and `streamline-gfx.h` headers exists to provide single headers for including all common headers in the respective modules. 

By executing the `sl::Application::Run` function, by providing a factory function for creating an application instance along with the command line arguments, the application instance will be created and the event loop will be started on the current thread. This will be a headless application with no GUI.

Users can inherit from `sl::Application` and return the user defined Application type from the factory function. `sl::Application` provides several customisation points for adding behaviour in the event loop.

If the graphics library is built, then the derived application class `sl::GuiApplication` exists for creating an Application with a GUI, with windowing using GLFW, a simple 2D renderer, with Dear ImGui for basic interfaces.

## Dependencies

* OpenSSL - for asio
* VulkanSDK - If building graphics module for compiling shaders
* Currently supports Windows and Linux (Ubuntu tested).

## License

This project is licensed under the MIT License - see the LICENSE.md file for details
