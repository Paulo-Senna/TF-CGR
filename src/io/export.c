/* export.c - Exportação da forma exibida (ver export.h). */
#include "io/export.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "geometry/morph.h"
#include "io/obj.h"

#define PATH_MAX_LEN 1024

static const char *base_name(const char *path) {
    const char *a = strrchr(path, '/'), *b = strrchr(path, '\\');
    if (b > a) a = b;
    return a ? a + 1 : path;
}

/* Extensão do arquivo em minúsculas, com o ponto (".obj"), ou "" se não tiver. */
static void extension(const char *path, char *out, size_t size) {
    const char *dot = strrchr(base_name(path), '.');
    size_t i;
    out[0] = '\0';
    if (!dot) return;
    for (i = 0; dot[i] && i + 1 < size; ++i) out[i] = (char)tolower((unsigned char)dot[i]);
    out[i] = '\0';
}

static int write_mtl(const char *path, Vec3 kd) {
    FILE *f = fopen(path, "w");
    if (!f) return 0;
    fprintf(f, "# Material exportado pelo TF-CGR\n");
    fprintf(f, "newmtl material\n");
    fprintf(f, "Ka 0.1 0.1 0.1\nKd %.4f %.4f %.4f\nKs 0.4 0.4 0.4\nNs 32\nd 1\nillum 2\n", kd.x,
            kd.y, kd.z);
    return fclose(f) == 0;
}

int export_model(const char *obj_path, const Shape *a, const Shape *b, float t, char *msg,
                 size_t msg_size) {
    char path[PATH_MAX_LEN], mtl[PATH_MAX_LEN];
    char ext[16], header[512];
    Vec3 *pos = (Vec3 *)malloc(VERTEX_COUNT * sizeof(Vec3));
    Vec3 *nrm = (Vec3 *)malloc(VERTEX_COUNT * sizeof(Vec3));
    unsigned *idx = (unsigned *)malloc(INDEX_COUNT * sizeof(unsigned));
    size_t stem_len;
    int ok = 0;

    if (!pos || !nrm || !idx) {
        snprintf(msg, msg_size, "Erro: memoria insuficiente para exportar.");
        goto done;
    }
    if (strlen(obj_path) + 8 >= PATH_MAX_LEN) {
        snprintf(msg, msg_size, "Erro: caminho do arquivo longo demais.");
        goto done;
    }

    /* Nome dos arquivos: "pasta/nome" + .obj e .mtl */
    extension(obj_path, ext, sizeof(ext));
    stem_len = strlen(obj_path);
    if (strcmp(ext, ".obj") == 0) {
        stem_len -= 4;
        snprintf(path, sizeof(path), "%s", obj_path);
    } else {
        snprintf(path, sizeof(path), "%s.obj", obj_path);
    }
    snprintf(mtl, sizeof(mtl), "%.*s.mtl", (int)stem_len, obj_path);

    /* Exatamente a mesma conta que gera a imagem da tela. */
    morph_blend(a, b, t, pos, nrm);
    grid_build_indices(idx);

    /* A primeira linha marca o arquivo como "já está na grade": se ele for
     * importado de volta, não precisa ser reamostrado. */
    snprintf(header, sizeof(header), "%s %d %d\n# Exportado pelo TF-CGR\n# %s -> %s, mistura %.0f%%",
             OBJ_GRID_MARK, GRID_U + 1, GRID_V + 1, a->name, b->name, t * 100.0f);

    if (!write_mtl(mtl, morph_color(a, b, t)) ||
        !obj_write(path, header, base_name(mtl), "material", pos, nrm, VERTEX_COUNT, idx,
                   INDEX_COUNT)) {
        snprintf(msg, msg_size, "Erro: nao foi possivel salvar %.200s", path);
        goto done;
    }
    snprintf(msg, msg_size, "Modelo salvo: %.250s", path);
    ok = 1;
done:
    free(pos);
    free(nrm);
    free(idx);
    return ok;
}
