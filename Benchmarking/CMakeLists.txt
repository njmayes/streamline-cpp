cmake_minimum_required(VERSION 3.20)

# Google Benchmark
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    googlebenchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.8.3
)

FetchContent_MakeAvailable(googlebenchmark)

# Benchmark executable
file(GLOB_RECURSE STREAMLINE_BENCHMARK_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_executable(streamline-benchmark
    ${STREAMLINE_BENCHMARK_SOURCES}
)

target_compile_features(streamline-benchmark PRIVATE cxx_std_23)

target_link_libraries(streamline-benchmark
    PRIVATE
        benchmark::benchmark
        benchmark::benchmark_main
		streamline::core
)


# Reasonable optimization defaults for benchmark builds
if (MSVC)
    target_compile_options(streamline-benchmark PRIVATE /W4 /O2 /DNDEBUG)
else()
    target_compile_options(streamline-benchmark PRIVATE -Wall -Wextra -Wpedantic -O3 -DNDEBUG)
endif()