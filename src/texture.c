/* texture.c - Texturas procedurais (geradas na CPU) e carregamento de imagens. */
#include "texture.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stb_image.h> /* implementação em external/stb/stb_image_impl.c */

const char *texture_name(int which) {
    static const char *names[TEX_COUNT] = {"Nenhuma", "Xadrez", "Listras", "Grade UV",
                                           "Imagem carregada"};
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

int texture_make_presets(GLuint ids[TEX_COUNT]) {
    unsigned char *px = (unsigned char *)malloc((size_t)TEX_PROC_SIZE * TEX_PROC_SIZE * 4);
    if (!px) return 0;
    memset(ids, 0, TEX_COUNT * sizeof(GLuint));
    make_checker(px);
    ids[TEX_CHECKER] = texture_create_rgba(px, TEX_PROC_SIZE, TEX_PROC_SIZE);
    make_stripes(px);
    ids[TEX_STRIPES] = texture_create_rgba(px, TEX_PROC_SIZE, TEX_PROC_SIZE);
    make_uv_grid(px);
    ids[TEX_UV_GRID] = texture_create_rgba(px, TEX_PROC_SIZE, TEX_PROC_SIZE);
    free(px);
    return 1;
}

/* Só o nome do arquivo, sem as pastas. */
static const char *base_name(const char *path) {
    const char *a = strrchr(path, '/'), *b = strrchr(path, '\\');
    if (b > a) a = b;
    return a ? a + 1 : path;
}

/* Traduz as mensagens mais comuns do stb_image. */
static const char *translate_reason(const char *reason) {
    static const char *table[][2] = {
        {"Unable to open file", "arquivo não encontrado ou sem permissão"},
        {"can't fopen", "arquivo não encontrado ou sem permissão"},
        {"Unknown image type", "formato não suportado (use PNG ou JPG)"},
        {"Image not of any known type, or corrupt", "formato não suportado ou arquivo corrompido"},
        {"Corrupt PNG", "PNG corrompido"},
        {"Corrupt JPEG", "JPG corrompido"},
        {"Out of memory", "memória insuficiente"},
    };
    size_t i;
    if (!reason) return "erro desconhecido";
    for (i = 0; i < sizeof(table) / sizeof(table[0]); ++i)
        if (strcmp(reason, table[i][0]) == 0) return table[i][1];
    return reason;
}

GLuint texture_load_file(const char *path, int *w, int *h, char *err, size_t err_size) {
    int n, x, y, max_size = 0;
    unsigned char *px;
    GLuint id;

    if (!path || !path[0]) {
        snprintf(err, err_size, "Informe o caminho de um arquivo PNG ou JPG.");
        return 0;
    }
    px = stbi_load(path, w, h, &n, 4); /* força 4 canais (RGBA) */
    if (!px) {
        snprintf(err, err_size, "Erro: %s (%.80s)", translate_reason(stbi_failure_reason()),
                 base_name(path));
        return 0;
    }
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_size);
    if (*w > max_size || *h > max_size) {
        snprintf(err, err_size, "Erro: imagem grande demais (%dx%d; máximo da placa: %d).", *w,
                 *h, max_size);
        stbi_image_free(px);
        return 0;
    }
    /* Espelha na horizontal: na grade, u cresce no sentido anti-horário
     * visto de cima, ou seja, da direita para a esquerda para quem olha a
     * forma de fora. Sem isso, textos da imagem apareceriam invertidos. */
    for (y = 0; y < *h; ++y) {
        unsigned char *row = px + (size_t)y * *w * 4;
        for (x = 0; x < *w / 2; ++x) {
            unsigned char tmp[4], *a = row + (size_t)x * 4, *b = row + (size_t)(*w - 1 - x) * 4;
            memcpy(tmp, a, 4);
            memcpy(a, b, 4);
            memcpy(b, tmp, 4);
        }
    }
    id = texture_create_rgba(px, *w, *h);
    stbi_image_free(px);
    return id;
}
