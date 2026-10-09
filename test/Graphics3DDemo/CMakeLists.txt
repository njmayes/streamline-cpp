cmake_minimum_required(VERSION 3.20)

if(NOT TARGET streamline::gfx)
    message(FATAL_ERROR "Graphics3DDemo requires streamline::gfx")
endif()

add_executable(streamline-graphics-3d-demo src/main.cpp)
target_compile_features(streamline-graphics-3d-demo PRIVATE cxx_std_23)
target_link_libraries(streamline-graphics-3d-demo PRIVATE streamline::gfx)

# Both renderers are initialized by Renderer::Init; deploy all their assets.
add_custom_target(streamline-graphics-3d-demo-copy-shaders ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:streamline-graphics-3d-demo>/resources/shaders"

    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/../ChatDemo/resources/shaders"
        "$<TARGET_FILE_DIR:streamline-graphics-3d-demo>/resources/shaders"

    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${STREAMLINE_GFX_3D_SHADER_DIR}"
        "$<TARGET_FILE_DIR:streamline-graphics-3d-demo>/resources/shaders"

    DEPENDS streamline-graphics-3d-demo
    VERBATIM
)