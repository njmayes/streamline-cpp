# streamline-cpp

This is a general purpose C++23 framework/library that contains a collection of shared code from various projects I have made over the years.

The basis of the library uses a mixture of code from my WIP game engine [Labyrinth Engine](https://github.com/amayesingnathan/LabyrinthEngine) 
as well as my C++20 Dear ImGui wrapper [imgui-cpp](https://github.com/amayesingnathan/imgui-cpp).

This is combined with various other bits of code and ideas, such as Rust-style Enums and Result types complete with a form of basic
pattern matching and Do notation, or a runtime reflection system, provides a selection of tools to save time rewriting application boilerplate code 
and reusing code ideas in different places.

The framework consists of two modules, a core module that provides the basic headless application framework as well as other utilities,
and an optional graphics module that provides windowing as well as a minimal 2D renderer API that uses OpenGL, combined with Dear ImGui for simple interfaces.

## Requirements

* OpenSSL - for asio 

#### Graphics Module Only
* VulkanSDK - For compiling shaders
* Python + Jinja2 - For glad OpenGL loader generation

## Installation

For use as a library, the framework currently only supports CMake, and is most easily consumed using `FetchContent`.

Two CMake options are provided:

* STREAMLINE_BUILD_GFX - Builds the optional graphics library
* STREAMLINE_BUILD_TEST_APP - Bulds the test app which contains some sample code (requires the graphics library).

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

There are `SL/Core.h` and `SL/Gfx.h` headers which provide single headers for including all common headers in their respective modules. 

The framework itself can be consumed as a basic set of utilities included for use in your program, or it can be run as an application directly by doing the following in your main function.

```cpp
#include <SL/Core.h>

sl::Application* CreateApplication( sl::CommandLineArgs args );

int main( int argc, char* argv[] )
{
	sl::CommandLineArgs args{ argc, argv };
	sl::Application::Run( CreateApplication, args );
}
```

Users should inherit from `sl::Application` and return the user defined Application type from the factory function. `sl::Application` provides several customisation points for adding behaviour in the event loop.

By executing the `sl::Application::Run` function, the application instance will be created and the event loop will be started on the current thread. By default this is a headless application with no GUI.

If the graphics library is built, then the derived application class `sl::GuiApplication` exists for creating an Application with a GUI, with windowing using GLFW, a simple 2D renderer, with Dear ImGui for basic interfaces.

### Test Application
The test application provided demonstrates a simple demo of both modules. The application functions as a chat room server or client, depending on command line parameters. 

There is not currently a way to run the chat room without SSL, so users must generate their own certficates and keys which should be placed in the same directory as the executable. You can create the cert/key pair by running:

```
openssl genpkey -algorithm RSA -out server.key -pkeyopt rsa_keygen_bits:2048

openssl req -new -x509 -key server.key -out server.crt -days 365 -sha256
```

#### Chat Room Server

The following is an example of the command line parameters you would use to run the test app as a chat room server:

```
./streamline-testapp server --ports=52612,62132 --cert=server.crt --key=server.key
```

This will run the server listening on ports 52612 and 62132, and use the `server.crt` and `server.key` certificate and private key respectively.

#### Chat Room Client

The following is an example of the command line parameters you would use to run the test app as a chat room client:

```
./streamline-testapp client --host=127.0.0.1 --port=52612 --cert=server.crt
```

This will attempt to connect to a host at address 127.0.0.1 (localhost) on port 52612 and will use the `server.crt` certificate as verification. 

## License

This project is licensed under the MIT License - see the LICENSE.md file for details
