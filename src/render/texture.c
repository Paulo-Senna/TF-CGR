/* texture.c - Texturas procedurais (geradas na CPU). */
#include "render/texture.h"

#include <stdlib.h>
#include <string.h>

const char *texture_name(int which) {
    static const char *names[TEX_COUNT] = {"Nenhuma", "Xadrez", "Listras", "Grade UV"};
    return (which >= 0 && which < TEX_COUNT) ? names[which] : "?";
}

GLuint texture_create_rgba(const unsigned char *pixels, int w, int h) {
    GLuint id = 0;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    /* GL_REPEAT: com o slider "Repetição" o (u,v) passa de 1 e a textura se repete. */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    /* Mipmaps + filtro trilinear: sem serrilhado quando a textura fica pequena na tela. */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    return id;
}

/* Escreve um pixel RGBA opaco. */
static void put(unsigned char *px, int x, int y, int r, int g, int b) {
    unsigned char *p = px + ((size_t)y * TEX_PROC_SIZE + x) * 4;
    p[0] = (unsigned char)r;
    p[1] = (unsigned char)g;
    p[2] = (unsigned char)b;
    p[3] = 255;
}

/* Xadrez 8x8: claro/escuro alternando em x e em y. */
static void make_checker(unsigned char *px) {
    const int cell = TEX_PROC_SIZE / 8;
    int x, y;
    for (y = 0; y < TEX_PROC_SIZE; ++y)
        for (x = 0; x < TEX_PROC_SIZE; ++x) {
            int light = ((x / cell) + (y / cell)) % 2 == 0;
            int c = light ? 235 : 45;
            put(px, x, y, c, c, c);
        }
}

/* 16 listras verticais, 8 claras e 8 escuras (variam em u = em torno do eixo Y).
 * Com torção elas viram hélices; com dobra, mostram o lado esticado. */
static void make_stripes(unsigned char *px) {
    const int stripe = TEX_PROC_SIZE / 16;
    int x, y;
    for (y = 0; y < TEX_PROC_SIZE; ++y)
        for (x = 0; x < TEX_PROC_SIZE; ++x) {
            if ((x / stripe) % 2 == 0) put(px, x, y, 240, 240, 240);
            else put(px, x, y, 60, 60, 70);
        }
}

/* Grade UV: vermelho cresce com u, verde cresce com v (cada ponto tem uma
 * cor única, então dá para ver para onde cada região da malha foi) e
 * linhas escuras a cada 1/8, que mostram as distorções da grade. */
static void make_uv_grid(unsigned char *px) {
    const int cell = TEX_PROC_SIZE / 8;
    int x, y;
    for (y = 0; y < TEX_PROC_SIZE; ++y)
        for (x = 0; x < TEX_PROC_SIZE; ++x) {
            int line = (x % cell) < 2 || (y % cell) < 2;
            int r = 40 + 215 * x / (TEX_PROC_SIZE - 1);
            int g = 40 + 215 * y / (TEX_PROC_SIZE - 1);
            if (line) put(px, x, y, 20, 20, 30);
            else put(px, x, y, r, g, 170);
        }
}

int texture_proc_pixels(int which, unsigned char *px) {
    switch (which) {
        case TEX_CHECKER: make_checker(px); return 1;
        case TEX_STRIPES: make_stripes(px); return 1;
        case TEX_UV_GRID: make_uv_grid(px); return 1;
        default: return 0;
    }
}

int texture_make_presets(GLuint ids[TEX_COUNT]) {
    unsigned char *px = (unsigned char *)malloc((size_t)TEX_PROC_SIZE * TEX_PROC_SIZE * 4);
    int i;
    if (!px) return 0;
    memset(ids, 0, TEX_COUNT * sizeof(GLuint));
    for (i = 0; i < TEX_COUNT; ++i)
        if (texture_proc_pixels(i, px)) ids[i] = texture_create_rgba(px, TEX_PROC_SIZE, TEX_PROC_SIZE);
    free(px);
    return 1;
}
