/* texture.h - Texturas procedurais (geradas na CPU).
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
 * ids[TEX_NONE] fica 0. Retorna 0 se faltar memória. */
int texture_make_presets(GLuint ids[TEX_COUNT]);


#endif /* TEXTURE_H */
