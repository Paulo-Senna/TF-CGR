/* export.c - Exportação do modelo deformado (ver export.h). */
#include "io/export.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app/app.h"
#include "geometry/deform.h"
#include "io/obj.h"

#define PATH_MAX_LEN 1024
#define STEM_MAX_LEN (PATH_MAX_LEN - 32) /* sobra para "_textura.jpeg" etc. */

static const char *base_name(const char *path) {
    const char *a = strrchr(path, '/'), *b = strrchr(path, '\\');
    if (b > a) a = b;
    return a ? a + 1 : path;
}

/* Extensão do arquivo em minúsculas, com o ponto (".png"), ou "" se não tiver. */
static void extension(const char *path, char *out, size_t size) {
    const char *dot = strrchr(base_name(path), '.');
    size_t i;
    out[0] = '\0';
    if (!dot) return;
    for (i = 0; dot[i] && i + 1 < size; ++i) out[i] = (char)tolower((unsigned char)dot[i]);
    out[i] = '\0';
}

/* Imagem TGA sem compressão (formato simples, aberto pelo Blender e afins).
 * px: RGBA, linha 0 = embaixo (igual à coordenada v da textura). */
static int write_tga(const char *path, const unsigned char *px, int w, int h) {
    unsigned char header[18] = {0};
    FILE *f = fopen(path, "wb");
    int i;
    if (!f) return 0;
    header[2] = 2; /* imagem colorida sem compressão */
    header[12] = (unsigned char)(w & 255);
    header[13] = (unsigned char)(w >> 8);
    header[14] = (unsigned char)(h & 255);
    header[15] = (unsigned char)(h >> 8);
    header[16] = 32; /* bits por pixel (BGRA) */
    header[17] = 8;  /* 8 bits de alfa; origem no canto de baixo */
    fwrite(header, 1, sizeof(header), f);
    for (i = 0; i < w * h; ++i) {
        const unsigned char *p = px + (size_t)i * 4;
        unsigned char bgra[4];
        bgra[0] = p[2]; bgra[1] = p[1]; bgra[2] = p[0]; bgra[3] = p[3];
        fwrite(bgra, 1, 4, f);
    }
    return fclose(f) == 0;
}

static int copy_file(const char *from, const char *to) {
    char buf[64 * 1024];
    size_t n;
    int ok = 1;
    FILE *in = fopen(from, "rb"), *out;
    if (!in) return 0;
    out = fopen(to, "wb");
    if (!out) {
        fclose(in);
        return 0;
    }
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0)
        if (fwrite(buf, 1, n, out) != n) ok = 0;
    fclose(in);
    return (fclose(out) == 0) && ok;
}

/* Salva a textura em uso ao lado do .obj. Escreve em 'tex_file' o caminho
 * criado (ou "" se não há textura). O flag 'flip' diz se o (u,v) precisa ser
 * espelhado para bater com a imagem original. Retorna 0 em caso de erro. */
static int export_texture(const char *stem, char *tex_file, size_t size, int *flip) {
    const TexState *tx = &app.tex;
    tex_file[0] = '\0';
    *flip = 0;
    if (tx->current == TEX_NONE || !tx->ids[tx->current]) return 1;

    if (tx->current == TEX_IMAGE) {
        /* Ao carregar, a imagem foi espelhada na horizontal e a linha de
         * cima virou v = 0 (ver texture.c). O arquivo copiado é o original,
         * então o (u,v) exportado é espelhado nos dois eixos. */
        char ext[16];
        extension(tx->path, ext, sizeof(ext));
        snprintf(tex_file, size, "%s_textura%s", stem, ext);
        *flip = 1;
        return copy_file(tx->path, tex_file);
    } else {
        unsigned char *px = (unsigned char *)malloc((size_t)TEX_PROC_SIZE * TEX_PROC_SIZE * 4);
        int ok;
        if (!px) return 0;
        texture_proc_pixels(tx->current, px);
        snprintf(tex_file, size, "%s_textura.tga", stem);
        ok = write_tga(tex_file, px, TEX_PROC_SIZE, TEX_PROC_SIZE);
        free(px);
        return ok;
    }
}

static int write_mtl(const char *path, Vec3 kd, const char *tex_file) {
    FILE *f = fopen(path, "w");
    if (!f) return 0;
    fprintf(f, "# Material exportado pelo Laboratorio de Deformacao\n");
    fprintf(f, "newmtl material\n");
    fprintf(f, "Ka 0.1 0.1 0.1\nKd %.4f %.4f %.4f\nKs 0.4 0.4 0.4\nNs 32\nd 1\nillum 2\n", kd.x,
            kd.y, kd.z);
    if (tex_file[0]) fprintf(f, "map_Kd %s\n", base_name(tex_file));
    return fclose(f) == 0;
}

int export_model(const char *obj_path) {
    const MorphState *m = &app.morph;
    const Shape *a = &app.shapes[m->from], *b = &app.shapes[m->to];
    const float t = app.morph.t;
    char path[PATH_MAX_LEN], stem[STEM_MAX_LEN], mtl[PATH_MAX_LEN], tex[PATH_MAX_LEN];
    char ext[16], header[512], text[256];
    Vec3 *pos = (Vec3 *)malloc(VERTEX_COUNT * sizeof(Vec3));
    Vec3 *nrm = (Vec3 *)malloc(VERTEX_COUNT * sizeof(Vec3));
    float *uv = (float *)malloc(VERTEX_COUNT * 2 * sizeof(float));
    unsigned *idx = (unsigned *)malloc(INDEX_COUNT * sizeof(unsigned));
    Vec3 kd;
    int i, flip, ok = 0;

    if (!pos || !nrm || !uv || !idx) {
        app_message(&app.export_msg, 1, "Erro: memória insuficiente para exportar.");
        goto done;
    }
    if (strlen(obj_path) + 1 >= STEM_MAX_LEN) {
        app_message(&app.export_msg, 1, "Erro: caminho do arquivo longo demais.");
        goto done;
    }

    /* Nome dos arquivos: "pasta/nome" + .obj / .mtl / _textura.xxx */
    extension(obj_path, ext, sizeof(ext));
    if (strcmp(ext, ".obj") == 0) {
        snprintf(path, sizeof(path), "%s", obj_path);
        snprintf(stem, sizeof(stem), "%.*s", (int)(strlen(obj_path) - 4), obj_path);
    } else {
        snprintf(path, sizeof(path), "%s.obj", obj_path);
        snprintf(stem, sizeof(stem), "%s", obj_path);
    }
    snprintf(mtl, sizeof(mtl), "%s.mtl", stem);

    if (!export_texture(stem, tex, sizeof(tex), &flip)) {
        app_message(&app.export_msg, 1, "Erro: não foi possível salvar a textura.");
        goto done;
    }

    /* A mesma conta do vertex shader: mistura A/B e depois as deformações. */
    grid_build_uvs(uv);
    grid_build_indices(idx);
    for (i = 0; i < VERTEX_COUNT; ++i) {
        Vec3 p = v3_lerp(a->positions[i], b->positions[i], t);
        Vec3 n = v3_normalize(v3_lerp(a->normals[i], b->normals[i], t));
        pos[i] = deform_point(p, &app.deform);
        nrm[i] = deform_normal(p, n, &app.deform);
        uv[2 * i] *= app.tex.scale;
        uv[2 * i + 1] *= app.tex.scale;
        if (flip) {
            uv[2 * i] = 1.0f - uv[2 * i];
            uv[2 * i + 1] = 1.0f - uv[2 * i + 1];
        }
    }

    /* Cor do material: a da forma, ou branco se a textura aparece pura. */
    kd = v3_lerp(a->color, b->color, t);
    if (tex[0] && !app.tex.multiply) kd = v3(1, 1, 1);

    snprintf(header, sizeof(header),
             "%s %d %d\n# Exportado pelo Laboratorio de Deformacao\n"
             "# %s -> %s, mistura %.0f%%, taper %.2f, twist %.2f, bend %.2f",
             OBJ_GRID_MARK, GRID_U + 1, GRID_V + 1, a->name, b->name, t * 100.0f,
             app.deform.taper, app.deform.twist, app.deform.bend);

    if (!write_mtl(mtl, kd, tex) ||
        !obj_write(path, header, base_name(mtl), "material", pos, nrm, uv, VERTEX_COUNT, idx,
                   INDEX_COUNT)) {
        snprintf(text, sizeof(text), "Erro: não foi possível salvar %.150s", path);
        app_message(&app.export_msg, 1, text);
        goto done;
    }
    snprintf(text, sizeof(text), "Modelo salvo: %.200s", path);
    app_message(&app.export_msg, 0, text);
    ok = 1;
done:
    free(pos);
    free(nrm);
    free(uv);
    free(idx);
    return ok;
}
