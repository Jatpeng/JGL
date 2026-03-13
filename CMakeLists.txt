cmake_minimum_required(VERSION 3.16)

project(JGL_MeshLoader
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(SRC_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/JGL_MeshLoader/source)

# 主工程源码（排除 3rdparty，避免自动把所有示例和后端都编进来）
file(GLOB_RECURSE APP_SOURCES
    ${SRC_ROOT}/*.cpp
)

file(GLOB_RECURSE APP_3RDPARTY_SOURCES
    ${SRC_ROOT}/3rdparty/*.cpp
)

list(REMOVE_ITEM APP_SOURCES ${APP_3RDPARTY_SOURCES})

# 移除测试用的独立 main，避免与正式入口冲突
list(REMOVE_ITEM APP_SOURCES
    ${SRC_ROOT}/elems/mian.cpp
)

# ImGui 核心源码
set(IMGUI_CORE_SOURCES
    ${SRC_ROOT}/3rdparty/imgui-docking/imgui.cpp
    ${SRC_ROOT}/3rdparty/imgui-docking/imgui_draw.cpp
    ${SRC_ROOT}/3rdparty/imgui-docking/imgui_widgets.cpp
    ${SRC_ROOT}/3rdparty/imgui-docking/imgui_tables.cpp
    ${SRC_ROOT}/3rdparty/imgui-docking/imgui_demo.cpp
)

# 只启用你实际使用的 ImGui 后端（GLFW + OpenGL3）
set(IMGUI_BACKEND_SOURCES
    ${SRC_ROOT}/3rdparty/imgui-docking/backends/imgui_impl_glfw.cpp
    ${SRC_ROOT}/3rdparty/imgui-docking/backends/imgui_impl_opengl3.cpp
)

# stb_image 实现文件
set(STB_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/STB_IMAGE/stb_image.cpp
)

add_executable(JGL_MeshLoader
    ${APP_SOURCES}
    ${IMGUI_CORE_SOURCES}
    ${IMGUI_BACKEND_SOURCES}
    ${STB_SOURCES}
)

target_include_directories(JGL_MeshLoader
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/JGL_MeshLoader/source
        ${CMAKE_CURRENT_SOURCE_DIR}/JGL_MeshLoader/include
        ${CMAKE_CURRENT_SOURCE_DIR}/JGL_MeshLoader/source/3rdparty/imgui-docking
        ${CMAKE_CURRENT_SOURCE_DIR}/JGL_MeshLoader/source/3rdparty/imgui-docking/backends
        ${CMAKE_CURRENT_SOURCE_DIR}/JGL_MeshLoader/source/3rdparty/plugins/imgui
        ${CMAKE_CURRENT_SOURCE_DIR}/STB_IMAGE
)

set(GLFW_LIB_DIR   ${CMAKE_CURRENT_SOURCE_DIR}/JGL_MeshLoader/libs/glfw)
set(ASSIMP_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/JGL_MeshLoader/libs/assimp)
set(GL_LIB_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/JGL_MeshLoader/libs/gl)

target_link_directories(JGL_MeshLoader
    PRIVATE
        ${GLFW_LIB_DIR}
        ${ASSIMP_LIB_DIR}
        ${GL_LIB_DIR}
)

find_package(OpenGL REQUIRED)

target_link_libraries(JGL_MeshLoader
    PRIVATE
        OpenGL::GL
        glfw3
        glew32s
        assimp
)

if(MSVC)
    target_precompile_headers(JGL_MeshLoader PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/JGL_MeshLoader/source/pch.h
    )
endif()

set_target_properties(JGL_MeshLoader PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

add_custom_command(TARGET JGL_MeshLoader POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/JGL_MeshLoader/libs/assimp/assimp-vc140-mt.dll"
        $<TARGET_FILE_DIR:JGL_MeshLoader>
)

