cmake_minimum_required(VERSION 3.10)
project(Morphing3D C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

add_executable(morph3d
    src/main.cpp
    src/shapes.cpp
    src/shader.cpp
    external/glad/src/gl.c
)
target_include_directories(morph3d PRIVATE src external/glad/include)
target_link_libraries(morph3d PRIVATE glfw OpenGL::GL)
# Permite rodar o executável de qualquer pasta (achar os shaders)
target_compile_definitions(morph3d PRIVATE SHADER_DIR="${CMAKE_SOURCE_DIR}/shaders")
