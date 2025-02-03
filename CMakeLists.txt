# 基本模板
cmake_minimum_required(VERSION 3.10)

# 使用相对路径
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/external/vcpkg/scripts/buildsystems/vcpkg.cmake")

# 配置项目
project(JGL)

# 查找包
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 REQUIRED)

add_executable(${PROJECT_NAME} 
    src/main.cpp
    src/pch.cpp
    src/application.cpp
)

# 链接库
target_link_libraries(${PROJECT_NAME} 
    PRIVATE 
    OpenGL::GL
    GLEW::GLEW
    glfw
)

# 包含头文件目录
target_include_directories(${PROJECT_NAME} 
    PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)