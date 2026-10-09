cmake_minimum_required(VERSION 3.20)

if(STREAMLINE_BUILD_BENCHMARK)
    add_subdirectory(Benchmark)
endif()

if(STREAMLINE_BUILD_TEST_APP)
    add_subdirectory(Sandbox)
endif()

if(STREAMLINE_BUILD_CHAT_DEMO)
    add_subdirectory(ChatDemo)
endif()