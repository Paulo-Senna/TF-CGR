/* obj.c - Leitura e escrita de arquivos .obj (ver obj.h). */
#include "io/obj.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Garante espaço para 'needed' itens num vetor dinâmico (dobra a capacidade). */
static int reserve(void **data, int *cap, int needed, size_t item) {
    void *p;
    int new_cap;
    if (needed <= *cap) return 1;
    new_cap = *cap ? *cap * 2 : 1024;
    while (new_cap < needed) new_cap *= 2;
    p = realloc(*data, (size_t)new_cap * item);
    if (!p) return 0;
    *data = p;
    *cap = new_cap;
    return 1;
}

static int push_vec3(Vec3 **arr, int *count, int *cap, const char *s) {
    Vec3 v = v3(0, 0, 0);
    char *end;
    if (!reserve((void **)arr, cap, *count + 1, sizeof(Vec3))) return 0;
    v.x = strtof(s, &end);
    v.y = strtof(end, &end);
    v.z = strtof(end, &end);
    (*arr)[(*count)++] = v;
    return 1;
}

void obj_free(ObjMesh *m) {
    free(m->positions);
    free(m->normals);
    free(m->tris);
    memset(m, 0, sizeof(*m));
}

int obj_read(const char *path, ObjMesh *m, char *err, size_t err_size) {
    static char line[8192];
    int vcap = 0, ncap = 0, tcap = 0, bad_index = 0, ok = 1, t;
    FILE *f = fopen(path, "r");

    memset(m, 0, sizeof(*m));
    if (!f) {
        snprintf(err, err_size, "Erro: não foi possível abrir o arquivo.");
        return 0;
    }
    while (ok && fgets(line, sizeof(line), f)) {
        if (strncmp(line, OBJ_GRID_MARK, strlen(OBJ_GRID_MARK)) == 0) m->grid_file = 1;
        else if (line[0] == 'v' && line[1] == ' ')
            ok = push_vec3(&m->positions, &m->vcount, &vcap, line + 2);
        else if (line[0] == 'v' && line[1] == 'n' && line[2] == ' ')
            ok = push_vec3(&m->normals, &m->ncount, &ncap, line + 3);
        else if (line[0] == 'f' && line[1] == ' ') {
            /* Face com n vértices: vira o leque (0,1,2), (0,2,3), ... */
            long first = 0, prev = 0;
            int n = 0;
            char *tok = strtok(line + 2, " \t\r\n");
            for (; tok && ok; tok = strtok(NULL, " \t\r\n"), ++n) {
                long idx = strtol(tok, NULL, 10); /* só o número antes da '/' */
                if (idx < 0) idx = m->vcount + idx;  /* índice relativo (do fim) */
                else idx -= 1;                       /* OBJ começa em 1 */
                if (idx < 0) bad_index = 1;
                if (n == 0) first = idx;
                else if (n >= 2) {
                    ok = reserve((void **)&m->tris, &tcap, m->tcount * 3 + 3, sizeof(unsigned));
                    if (ok) {
                        unsigned *tri = &m->tris[m->tcount * 3];
                        tri[0] = (unsigned)first;
                        tri[1] = (unsigned)prev;
                        tri[2] = (unsigned)idx;
                        m->tcount++;
                    }
                }
                prev = idx;
            }
        }
    }
    fclose(f);

    if (!ok) {
        snprintf(err, err_size, "Erro: memória insuficiente para ler o modelo.");
        obj_free(m);
        return 0;
    }
    for (t = 0; t < m->tcount * 3 && !bad_index; ++t)
        if (m->tris[t] >= (unsigned)m->vcount) bad_index = 1;
    if (m->vcount == 0 || m->tcount == 0) {
        snprintf(err, err_size, "Erro: o arquivo não tem vértices/faces de um modelo OBJ.");
        obj_free(m);
        return 0;
    }
    if (bad_index) {
        snprintf(err, err_size, "Erro: arquivo OBJ inválido (face com índice inexistente).");
        obj_free(m);
        return 0;
    }
    return 1;
}

int obj_write(const char *path, const char *header, const char *mtl_file, const char *material,
              const Vec3 *pos, const Vec3 *nrm, int vcount,
              const unsigned *idx, int icount) {
    FILE *f = fopen(path, "w");
    int i;
    if (!f) return 0;
    if (header) fprintf(f, "%s\n", header);
    if (mtl_file) fprintf(f, "mtllib %s\n", mtl_file);
    fprintf(f, "o modelo\n");
    for (i = 0; i < vcount; ++i) fprintf(f, "v %.6f %.6f %.6f\n", pos[i].x, pos[i].y, pos[i].z);
    for (i = 0; i < vcount; ++i) fprintf(f, "vn %.6f %.6f %.6f\n", nrm[i].x, nrm[i].y, nrm[i].z);
    if (material) fprintf(f, "usemtl %s\n", material);
    for (i = 0; i + 2 < icount; i += 3) {
        unsigned a = idx[i] + 1, b = idx[i + 1] + 1, c = idx[i + 2] + 1;
        fprintf(f, "f %u//%u %u//%u %u//%u\n", a, a, b, b, c, c); /* posição//normal */
    }
    return fclose(f) == 0;
}
