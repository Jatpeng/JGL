cmake_minimum_required(VERSION 3.16)

project(JGL_Engine
    VERSION 0.1.0
    LANGUAGES CXX
)

option(JGL_ENABLE_PYTHON "Build Python bindings" ON)

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(JGL_SCRIPT_WORKSPACE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Game)
set(JGL_SCRIPT_ENGINE_DIR ${JGL_SCRIPT_WORKSPACE_DIR}/engine)
set(JGL_ENGINE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/JGL_Engine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(SRC_ROOT ${JGL_ENGINE_DIR}/source)

file(GLOB_RECURSE CORE_SOURCES
    CONFIGURE_DEPENDS
    ${SRC_ROOT}/*.cpp
)

file(GLOB_RECURSE APP_3RDPARTY_SOURCES
    CONFIGURE_DEPENDS
    ${SRC_ROOT}/3rdparty/*.cpp
)

file(GLOB_RECURSE PYTHON_BINDING_SOURCES
    CONFIGURE_DEPENDS
    ${SRC_ROOT}/python/*.cpp
)

list(REMOVE_ITEM CORE_SOURCES ${APP_3RDPARTY_SOURCES})
list(REMOVE_ITEM CORE_SOURCES ${SRC_ROOT}/main.cpp)
list(REMOVE_ITEM CORE_SOURCES ${SRC_ROOT}/elems/mian.cpp)
list(REMOVE_ITEM CORE_SOURCES ${PYTHON_BINDING_SOURCES})

set(IMGUI_CORE_SOURCES
    ${SRC_ROOT}/3rdparty/imgui-docking/imgui.cpp
    ${SRC_ROOT}/3rdparty/imgui-docking/imgui_draw.cpp
    ${SRC_ROOT}/3rdparty/imgui-docking/imgui_widgets.cpp
    ${SRC_ROOT}/3rdparty/imgui-docking/imgui_tables.cpp
    ${SRC_ROOT}/3rdparty/imgui-docking/imgui_demo.cpp
)

set(IMGUI_BACKEND_SOURCES
    ${SRC_ROOT}/3rdparty/imgui-docking/backends/imgui_impl_glfw.cpp
    ${SRC_ROOT}/3rdparty/imgui-docking/backends/imgui_impl_opengl3.cpp
)

set(STB_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/STB_IMAGE/stb_image.cpp
)

add_library(jgl_core STATIC
    ${CORE_SOURCES}
    ${IMGUI_CORE_SOURCES}
    ${IMGUI_BACKEND_SOURCES}
    ${STB_SOURCES}
)

set_target_properties(jgl_core PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

target_include_directories(jgl_core
    PUBLIC
        ${JGL_ENGINE_DIR}/source
        ${JGL_ENGINE_DIR}/include
        ${JGL_ENGINE_DIR}/source/3rdparty/imgui-docking
        ${JGL_ENGINE_DIR}/source/3rdparty/imgui-docking/backends
        ${CMAKE_CURRENT_SOURCE_DIR}/STB_IMAGE
)

find_package(OpenGL REQUIRED)

set(JGL_PLATFORM_LIBS)
set(JGL_PLATFORM_INCLUDE_DIRS)

if(TARGET OpenGL::GL)
    set(JGL_OPENGL_LIB OpenGL::GL)
elseif(TARGET OpenGL::OpenGL)
    set(JGL_OPENGL_LIB OpenGL::OpenGL)
elseif(OPENGL_gl_LIBRARY)
    set(JGL_OPENGL_LIB ${OPENGL_gl_LIBRARY})
else()
    message(FATAL_ERROR "Failed to locate an OpenGL target or library.")
endif()

if(WIN32)
    set(GLFW_LIB_DIR   ${JGL_ENGINE_DIR}/libs/glfw)
    set(ASSIMP_LIB_DIR ${JGL_ENGINE_DIR}/libs/assimp)
    set(GL_LIB_DIR     ${JGL_ENGINE_DIR}/libs/gl)

    target_link_directories(jgl_core
        PUBLIC
            ${GLFW_LIB_DIR}
            ${ASSIMP_LIB_DIR}
            ${GL_LIB_DIR}
    )

    list(APPEND JGL_PLATFORM_LIBS
        glfw3
        glew32s
        assimp
    )
else()
    find_package(glfw3 REQUIRED)
    find_package(GLEW REQUIRED)
    find_package(assimp REQUIRED)

    if(TARGET glfw)
        list(APPEND JGL_PLATFORM_LIBS glfw)
    elseif(TARGET glfw3)
        list(APPEND JGL_PLATFORM_LIBS glfw3)
    elseif(DEFINED GLFW3_LIBRARY)
        list(APPEND JGL_PLATFORM_LIBS ${GLFW3_LIBRARY})
    elseif(DEFINED glfw3_LIBRARY)
        list(APPEND JGL_PLATFORM_LIBS ${glfw3_LIBRARY})
    else()
        message(FATAL_ERROR "Failed to locate a GLFW target or library.")
    endif()

    if(TARGET GLEW::GLEW)
        list(APPEND JGL_PLATFORM_LIBS GLEW::GLEW)
    elseif(TARGET GLEW::glew)
        list(APPEND JGL_PLATFORM_LIBS GLEW::glew)
    elseif(GLEW_LIBRARIES)
        list(APPEND JGL_PLATFORM_LIBS ${GLEW_LIBRARIES})
        list(APPEND JGL_PLATFORM_INCLUDE_DIRS ${GLEW_INCLUDE_DIRS})
    else()
        message(FATAL_ERROR "Failed to locate a GLEW target or library.")
    endif()

    if(TARGET assimp::assimp)
        list(APPEND JGL_PLATFORM_LIBS assimp::assimp)
    elseif(TARGET assimp)
        list(APPEND JGL_PLATFORM_LIBS assimp)
    elseif(ASSIMP_LIBRARIES)
        list(APPEND JGL_PLATFORM_LIBS ${ASSIMP_LIBRARIES})
        list(APPEND JGL_PLATFORM_INCLUDE_DIRS ${ASSIMP_INCLUDE_DIRS})
    else()
        message(FATAL_ERROR "Failed to locate an assimp target or library.")
    endif()

    if(UNIX AND NOT APPLE)
        list(APPEND JGL_PLATFORM_LIBS dl)
    endif()
endif()

if(JGL_PLATFORM_INCLUDE_DIRS)
    list(REMOVE_DUPLICATES JGL_PLATFORM_INCLUDE_DIRS)
    target_include_directories(jgl_core PUBLIC ${JGL_PLATFORM_INCLUDE_DIRS})
endif()

target_link_libraries(jgl_core
    PUBLIC
        ${JGL_OPENGL_LIB}
        ${JGL_PLATFORM_LIBS}
)

if(WIN32)
    target_link_libraries(jgl_core PUBLIC comdlg32)
endif()

if(MSVC)
    target_compile_options(jgl_core PRIVATE /FS)
    target_precompile_headers(jgl_core PRIVATE
        ${JGL_ENGINE_DIR}/source/pch.h
    )
endif()

add_executable(JGL_Engine
    ${SRC_ROOT}/main.cpp
)

target_link_libraries(JGL_Engine PRIVATE jgl_core)

set_target_properties(JGL_Engine PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

function(copy_assimp_runtime target_name)
    if(NOT WIN32)
        return()
    endif()

    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${JGL_ENGINE_DIR}/libs/assimp/assimp-vc140-mt.dll"
            $<TARGET_FILE_DIR:${target_name}>
    )
endfunction()

function(sync_script_workspace target_name)
    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "${JGL_SCRIPT_ENGINE_DIR}/$<CONFIG>"
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "${JGL_SCRIPT_WORKSPACE_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/Assets"
            "${JGL_SCRIPT_WORKSPACE_DIR}/Assets"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${JGL_ENGINE_DIR}/shaders"
            "${JGL_SCRIPT_WORKSPACE_DIR}/Shaders"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:${target_name}>"
            "${JGL_SCRIPT_ENGINE_DIR}/$<CONFIG>"
    )

    if(WIN32)
        add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${JGL_ENGINE_DIR}/libs/assimp/assimp-vc140-mt.dll"
                "${JGL_SCRIPT_ENGINE_DIR}/$<CONFIG>"
        )
    endif()
endfunction()

copy_assimp_runtime(JGL_Engine)
sync_script_workspace(JGL_Engine)

if(JGL_ENABLE_PYTHON)
    set(PYBIND11_FINDPYTHON ON)
    find_package(Python3 COMPONENTS Interpreter Development.Module REQUIRED)
    find_package(pybind11 CONFIG QUIET)

    if(NOT pybind11_FOUND)
        include(FetchContent)
        FetchContent_Declare(
            pybind11
            URL https://github.com/pybind/pybind11/archive/refs/tags/v2.13.6.zip
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
        FetchContent_MakeAvailable(pybind11)
    endif()

    pybind11_add_module(pyjgl
        ${PYTHON_BINDING_SOURCES}
    )

    target_link_libraries(pyjgl PRIVATE jgl_core)

    set_target_properties(pyjgl PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )

    copy_assimp_runtime(pyjgl)
    sync_script_workspace(pyjgl)
endif()

if(NOT Python3_EXECUTABLE)
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
endif()

add_custom_target(api_docs
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_api_docs.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating JGL_Engine API markdown docs"
)
