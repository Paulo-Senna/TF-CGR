/* texture.h - Texturas procedurais (geradas na CPU) e carregamento de imagens.
 *
 * As coordenadas de textura são o próprio (u, v) da grade paramétrica,
 * iguais para todas as formas. Por isso a textura "acompanha" a malha
 * durante a mistura e as deformações: um xadrez mostra exatamente onde a
 * superfície estica (quadrados grandes) ou comprime (quadrados pequenos). */
#ifndef TEXTURE_H
#define TEXTURE_H

#include <glad/gl.h>
#include <stddef.h>

enum {
    TEX_NONE = 0, /* sem textura: só a cor da forma */
    TEX_CHECKER,  /* xadrez */
    TEX_STRIPES,  /* listras na direção u */
    TEX_UV_GRID,  /* grade com gradiente de cor em u e v */
    TEX_IMAGE,    /* imagem PNG/JPG carregada pelo usuário */
    TEX_COUNT
};

#define TEX_PROC_SIZE 256 /* lado (em pixels) das texturas procedurais */

const char *texture_name(int which);

/* Cria uma textura RGBA (GL_REPEAT, mipmaps, filtro linear). */
GLuint texture_create_rgba(const unsigned char *pixels, int w, int h);

/* Desenha a textura procedural 'which' em px (TEX_PROC_SIZE² pixels RGBA;
 * a linha 0 é v = 0). Retorna 0 se 'which' não for procedural. */
int texture_proc_pixels(int which, unsigned char *px);

/* Gera as texturas procedurais em ids[TEX_CHECKER..TEX_UV_GRID].
 * ids[TEX_NONE] e ids[TEX_IMAGE] ficam 0. Retorna 0 se faltar memória. */
int texture_make_presets(GLuint ids[TEX_COUNT]);

/* Carrega um PNG/JPG com stb_image. Em caso de erro retorna 0 e escreve
 * a mensagem em 'err'. Em caso de sucesso, devolve largura/altura. */
GLuint texture_load_file(const char *path, int *w, int *h, char *err, size_t err_size);

#endif /* TEXTURE_H */
