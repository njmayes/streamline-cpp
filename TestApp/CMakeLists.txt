cmake_minimum_required(VERSION 3.20)

# Test application that uses both core and gfx modules.

# Ensure both modules are available (top-level must have added their subdirs)
if (NOT TARGET streamline::core)
    message(FATAL_ERROR "TestApp requires target 'streamline::core' – make sure StreamlineCore is added before TestApp.")
endif()

if (NOT TARGET streamline::gfx)
    message(FATAL_ERROR "TestApp requires target 'streamline::gfx' – set STREAMLINE_BUILD_GFX=ON and add StreamlineGfx before TestApp.")
endif()

project(streamline-testapp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Collect sources (adjust pattern if you have a different layout)
file(GLOB_RECURSE STREAMLINE_TESTAPP_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_executable(streamline-testapp
    ${STREAMLINE_TESTAPP_SOURCES}
)

target_compile_features(streamline-testapp PUBLIC cxx_std_23)

# Link against both modules
target_link_libraries(streamline-testapp
    PRIVATE
        streamline::core
        streamline::gfx
        streamline::net
)
