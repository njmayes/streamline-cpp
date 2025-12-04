cmake_minimum_required(VERSION 3.20)

# -------------------------------------------------------------
# Graphics library
# -------------------------------------------------------------

add_library(streamline-gfx)
add_library(streamline::gfx ALIAS streamline-gfx)

target_compile_features(streamline-gfx PUBLIC cxx_std_23)

target_include_directories(streamline-gfx
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

file(GLOB_RECURSE STREAMLINE_GFX_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/*.cpp"
)

target_sources(streamline-gfx PRIVATE ${STREAMLINE_GFX_SOURCES})

target_link_libraries(streamline-gfx
    PUBLIC
        streamline::core
)

target_compile_definitions(streamline-gfx
    PUBLIC
        STREAMLINE_HAS_GFX=1
)


# -------------------------------------------------------------
# Third-party dependencies (graphics) via FetchContent
# -------------------------------------------------------------

# -------------------------
# stb
# -------------------------

FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(stb)

add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${stb_SOURCE_DIR})

# -------------------------
# GLFW
# -------------------------

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
)
FetchContent_MakeAvailable(glfw)

# -------------------------
# glad (OpenGL loader)
# -------------------------

FetchContent_Declare(
    glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG        v0.1.36
)
FetchContent_MakeAvailable(glad)

# -------------------------
# ImGui
# -------------------------

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        docking
)
FetchContent_MakeAvailable(imgui)

set(IMGUI_SOURCES
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

add_library(imgui_glfw STATIC ${IMGUI_SOURCES})

target_include_directories(imgui_glfw
    PUBLIC
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
)

target_link_libraries(imgui_glfw
    PUBLIC
        glfw
        glad
)

# -------------------------
# ImGuizmo
# -------------------------

FetchContent_Declare(
    imguizmo
    GIT_REPOSITORY https://github.com/CedricGuillemet/ImGuizmo.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(imguizmo)

add_library(imguizmo STATIC
    ${imguizmo_SOURCE_DIR}/ImGuizmo.cpp
)

target_include_directories(imguizmo
    PUBLIC
        ${imguizmo_SOURCE_DIR}
)
target_link_libraries(imguizmo
    PUBLIC
        imgui_glfw
)

# Attach gfx deps to streamline-gfx
target_link_libraries(streamline-gfx
    PUBLIC
        stb
        glfw
        glad
        imgui_glfw
        imguizmo
)

# -------------------------------------------------------------
# Vulkan / OpenGL / OpenSSL / shaderc / SPIRV-Cross / system libs
# -------------------------------------------------------------

# Vulkan SDK (includes + libs)
find_package(Vulkan REQUIRED)

# OpenGL (instead of raw opengl32/GL)
find_package(OpenGL REQUIRED)

target_link_libraries(streamline-gfx
    PUBLIC
        Vulkan::Vulkan
        OpenGL::GL
)

# shaderc / SPIRV-Cross:

if (MSVC)
    set(VULKAN_SDK "$ENV{VULKAN_SDK}")
    set(SHADERC_LIB_DIR     "${VULKAN_SDK}/Lib")
    set(SPIRV_CROSS_LIB_DIR "${VULKAN_SDK}/Lib")

    target_link_libraries(streamline-gfx
        PUBLIC
            $<$<CONFIG:Debug>:${SHADERC_LIB_DIR}/shaderc_sharedd.lib>
            $<$<CONFIG:Release>:${SHADERC_LIB_DIR}/shaderc_shared.lib>

            $<$<CONFIG:Debug>:${SPIRV_CROSS_LIB_DIR}/spirv-cross-cored.lib>
            $<$<CONFIG:Release>:${SPIRV_CROSS_LIB_DIR}/spirv-cross-core.lib>

            $<$<CONFIG:Debug>:${SPIRV_CROSS_LIB_DIR}/spirv-cross-glsld.lib>
            $<$<CONFIG:Release>:${SPIRV_CROSS_LIB_DIR}/spirv-cross-glsl.lib>
    )
else()
    target_link_libraries(streamline-gfx
        PUBLIC
            shaderc_shared
            spirv-cross-core
            spirv-cross-glsl
    )
endif()