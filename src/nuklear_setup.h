/* nuklear_setup.h - Inclui o Nuklear (interface gráfica) e o backend
 * GLFW + OpenGL 3 com as MESMAS opções em todos os arquivos que o usam.
 * A implementação é gerada uma única vez em external/nuklear/nuklear_impl.c. */
#ifndef NUKLEAR_SETUP_H
#define NUKLEAR_SETUP_H

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT

/* O backend chama funções do OpenGL: o glad precisa vir antes de tudo. */
#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <nuklear.h>
#include <nuklear_glfw_gl3.h>

#endif /* NUKLEAR_SETUP_H */
