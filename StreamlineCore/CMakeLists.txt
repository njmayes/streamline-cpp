cmake_minimum_required(VERSION 3.20)

# Library target
add_library(streamline-core)
add_library(streamline::core ALIAS streamline-core)

target_compile_features(streamline-core PUBLIC cxx_std_23)

target_include_directories(streamline-core
    PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/src
)

file(GLOB_RECURSE STREAMLINE_CORE_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

target_sources(streamline-core
    PRIVATE
        ${STREAMLINE_CORE_SOURCES}
)

target_compile_definitions(streamline-core
    PUBLIC
        $<$<CONFIG:Debug>:SL_DEBUG=1>
)

# -------------------------------------------------------------
# Core dependencies via FetchContent
# -------------------------------------------------------------

# nlohmann_json (header-only, has CMake, target nlohmann_json::nlohmann_json) 
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)


# glm (header-only, target glm::glm)
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
)

set(GLM_BUILD_LIBRARY OFF CACHE BOOL "" FORCE)
set(GLM_BUILD_TESTS   OFF CACHE BOOL "" FORCE)
set(GLM_BUILD_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(glm)


# magic_enum (header-only; uses magic_enum::magic_enum when available) 
FetchContent_Declare(
    magic_enum
    GIT_REPOSITORY https://github.com/Neargye/magic_enum.git
    GIT_TAG        v0.9.7
)
FetchContent_MakeAvailable(magic_enum)

# Some distros call the target magic_enum::magic_enum, some just magic_enum.
if(TARGET magic_enum::magic_enum)
    set(MAGIC_ENUM_TARGET magic_enum::magic_enum)
elseif(TARGET magic_enum)
    set(MAGIC_ENUM_TARGET magic_enum)
else()
    add_library(magic_enum INTERFACE)
    target_include_directories(magic_enum INTERFACE ${magic_enum_SOURCE_DIR}/include)
    set(MAGIC_ENUM_TARGET magic_enum)
endif()


# portable-file-dialogs (single header)
FetchContent_Declare(
    portable_file_dialogs
    GIT_REPOSITORY https://github.com/njmayes/portable-file-dialogs
    GIT_TAG        main
)
FetchContent_MakeAvailable(portable_file_dialogs)

if(NOT TARGET portable-file-dialogs)
    # Create a simple header-only target around the header
    add_library(portable-file-dialogs INTERFACE)
    target_include_directories(portable-file-dialogs
        INTERFACE
            ${portable_file_dialogs_SOURCE_DIR}
    )
endif()


target_link_libraries(streamline-core
    PUBLIC
        nlohmann_json::nlohmann_json
        glm::glm
        ${MAGIC_ENUM_TARGET}
        portable-file-dialogs
)