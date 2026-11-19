/* shader.h - Carregamento e compilação de shaders GLSL. */
#ifndef SHADER_H
#define SHADER_H

#include <glad/gl.h>
#include <stddef.h>

/* Lê os arquivos, compila e linka o programa.
 * Retorna 0 (e mostra o erro no terminal) se algo falhar. */
GLuint shader_load_program(const char *vert_path, const char *frag_path);

/* Procura um arquivo de shader em alguns diretórios comuns
 * (./shaders, ../shaders e o diretório definido na compilação)
 * e escreve o caminho encontrado em 'out'. */
void shader_find(const char *file_name, char *out, size_t out_size);

#endif /* SHADER_H */
