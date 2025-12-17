# streamline-cpp

A general purpose C++23 desktop application framework.

## Description

This is a general purpose C++23 framework/library that contains a collection of shared code from various projects I have made over the years.

The core of the library uses a mixture of code from my WIP game engine [Labyrinth Engine](https://github.com/amayesingnathan/LabyrinthEngine) 
as well as my C++20 Dear ImGui wrapper [imgui-cpp](https://github.com/amayesingnathan/imgui-cpp).

This is combined with various other common bits of code and ideas, such as Rust-style Enums and Result types complete with a form of basic
pattern matching and Do notation, or a runtime reflection system, provides a selection of tools to save time rewriting application boilerplate code 
and reusing code ideas in different places.

The core framework can be run as an application by including `SL/Core/Common/EntryPoint.h` and defining a `sl::Application* CreateApplication( int argc, char** argv )` function, 
or it can be used standalone as a collection of utilities.

The graphics module can be optionally built, depending on whether a GUI is required, and provides a minimal 2D renderer API that uses OpenGL as well as Dear ImGui for simple interfaces.
  
### Dependencies

* Python3
* OpenSSL - for asio
* VulkanSDK - If building graphics module for compiling shaders
* Currently supports Windows and Linux (Ubuntu tested).

## License

This project is licensed under the MIT License - see the LICENSE.md file for details
