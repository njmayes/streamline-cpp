cmake_minimum_required(VERSION 3.20)
project(streamline-cpp LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# -------------------------------------------------------------
# Options
# -------------------------------------------------------------

# Turn OFF to build "headless" (core-only) – no Vulkan/GLFW/Glad required
option(STREAMLINE_BUILD_GFX "Build streamline-gfx (graphics/windowing backends)" ON)

# Build the TestApp executable (example app)
option(STREAMLINE_BUILD_TEST_APP "Build Streamline TestApp example" ON)

if(NOT CMAKE_CONFIGURATION_TYPES)
    set(default_build_type "Debug")

    if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
        message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
        set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE STRING "Choose the type of build." FORCE)

        set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
            "Debug" "Release" "Profile")
    endif()
endif()

if (MSVC)
    add_compile_options(/Zc:preprocessor)
endif()


add_subdirectory(StreamlineCore)

if(STREAMLINE_BUILD_GFX)
    add_subdirectory(StreamlineGfx)
endif()

if(STREAMLINE_BUILD_TEST_APP)
    add_subdirectory(TestApp)
endif()