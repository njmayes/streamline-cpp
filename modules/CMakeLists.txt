cmake_minimum_required(VERSION 3.20)

add_subdirectory(Core)

if(STREAMLINE_BUILD_NET)
    add_subdirectory(Net)
endif()

if(STREAMLINE_BUILD_GFX)
    add_subdirectory(Graphics)
endif()

if(STREAMLINE_BUILD_CHAT_DEMO)
    add_subdirectory(ChatDemo)
endif()

if(STREAMLINE_BUILD_TEST_APP)
    add_subdirectory(Test)
endif()

if(STREAMLINE_BUILD_BENCHMARK)
    add_subdirectory(Benchmark)
endif()