# streamline-cpp

A general purpose C++20 desktop application framework.

## Description

This is a general purpose C++20 framework/library, that contains a collection of shared code from various projects I have made over the years.

The core application framework uses a mixture of code from my WIP game engine [Labyrinth Engine](https://github.com/amayesingnathan/LabyrinthEngine) 
as well as my C++20 Dear ImGui wrapper [imgui-cpp](https://github.com/amayesingnathan/imgui-cpp).

This core framework combined with various other common bits of code and ideas, such as Rust-style Result types, complete with a form of basic
pattern matching and Do notation, provides a selection of tools to save time rewriting application boilerplate code and reusing code ideas in 
different places.
  
### Dependencies

* Python3
* premake5 - Will be downloaded and extracted as a binary when the setup script is run if it is not found.
* glfw - Used for multi-platform windowing (built from source)
* Dear ImGui - Used for UI widgets (built from source)
* Currently only supports Windows and Linux (Ubuntu tested).

## License

This project is licensed under the MIT License - see the LICENSE.md file for details