cmake_minimum_required(VERSION 3.20)

add_subdirectory(Core)

if(STREAMLINE_BUILD_NET)
    add_subdirectory(Net)
endif()

if(STREAMLINE_BUILD_GFX)
    add_subdirectory(Graphics)
endif()