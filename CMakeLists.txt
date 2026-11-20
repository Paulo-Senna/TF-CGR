cmake_minimum_required(VERSION 3.10)
project(Morphing3D C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

# glad (carregador do OpenGL): código gerado, compilado à parte sem avisos.
add_library(glad STATIC external/glad/src/gl.c)
target_include_directories(glad SYSTEM PUBLIC external/glad/include)
target_compile_options(glad PRIVATE -w)

add_executable(morph3d
    src/main.c
    src/app/app.c
    src/geometry/shapes.c
    src/geometry/mesh_fit.c
    src/geometry/morph.c
    src/render/shader.c
    src/render/render.c
    src/io/obj.c
    src/io/export.c
    src/io/file_dialog.c
)
target_include_directories(morph3d PRIVATE src)
target_compile_options(morph3d PRIVATE -Wall -Wextra)
target_link_libraries(morph3d PRIVATE glad glfw OpenGL::GL ${CMAKE_DL_LIBS})
if(WIN32)
    target_link_libraries(morph3d PRIVATE comdlg32) # janela de abrir/salvar arquivo
else()
    target_link_libraries(morph3d PRIVATE m)
endif()
# Permite rodar o executável de qualquer pasta (achar os shaders)
target_compile_definitions(morph3d PRIVATE SHADER_DIR="${CMAKE_SOURCE_DIR}/shaders")
