cmake_minimum_required(VERSION 3.20)

# -------------------------------------------------------------
# Graphics library
# -------------------------------------------------------------

add_library(streamline-net)
add_library(streamline::net ALIAS streamline-net)

target_compile_features(streamline-net PUBLIC cxx_std_23)

target_include_directories(streamline-net
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

file(GLOB_RECURSE STREAMLINE_NET_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

target_sources(streamline-net PRIVATE ${STREAMLINE_NET_SOURCES})

target_link_libraries(streamline-net
    PUBLIC
        streamline::core
)

target_compile_definitions(streamline-net
    PUBLIC
        STREAMLINE_HAS_NET=1
)


# -------------------------------------------------------------
# Third-party dependencies (networking) via FetchContent
# -------------------------------------------------------------

# asio (standalone; treat as header-only interface)
FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG        asio-1-30-2
)
FetchContent_MakeAvailable(asio)

if(NOT TARGET asio)
  add_library(asio INTERFACE)
  target_include_directories(asio INTERFACE ${asio_SOURCE_DIR}/asio/include)
endif()

find_package(OpenSSL REQUIRED)

target_link_libraries(streamline-net
    PUBLIC
        asio
        OpenSSL::SSL
        OpenSSL::Crypto
)



# -------------------------------------------------------------
# Platform-specific system libraries
# -------------------------------------------------------------

if (WIN32)
    target_link_libraries(streamline-net
        PUBLIC
            Ws2_32
            Winmm
            Version
            Bcrypt
    )
elseif(UNIX AND NOT APPLE)
    target_link_libraries(streamline-net
        PUBLIC
            dl
    )
endif()
