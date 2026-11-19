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

# Bibliotecas de terceiros: compiladas à parte, sem avisos.
add_library(external STATIC
    external/glad/src/gl.c
    external/nuklear/nuklear_impl.c
    external/stb/stb_image_impl.c
)
target_include_directories(external SYSTEM PUBLIC
    external/glad/include external/nuklear external/stb)
target_include_directories(external PRIVATE src)
target_link_libraries(external PUBLIC glfw)
target_compile_options(external PRIVATE -w)

add_executable(morph3d
    src/main.c
    src/app/app.c
    src/geometry/shapes.c
    src/geometry/deform.c
    src/geometry/mesh_fit.c
    src/render/shader.c
    src/render/texture.c
    src/io/obj.c
    src/io/export.c
    src/io/file_dialog.c
    src/ui/ui.c
)
target_include_directories(morph3d PRIVATE src)
target_compile_options(morph3d PRIVATE -Wall -Wextra)
target_link_libraries(morph3d PRIVATE external glfw OpenGL::GL ${CMAKE_DL_LIBS})
if(WIN32)
    target_link_libraries(morph3d PRIVATE comdlg32) # janela de abrir/salvar arquivo
else()
    target_link_libraries(morph3d PRIVATE m)
endif()
# Permite rodar o executável de qualquer pasta (achar os shaders)
target_compile_definitions(morph3d PRIVATE SHADER_DIR="${CMAKE_SOURCE_DIR}/shaders")
