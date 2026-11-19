/* mesh_fit.c - Reamostragem de um modelo 3D na grade (ver mesh_fit.h). */
#include "geometry/mesh_fit.h"

#include <stdlib.h>
#include <string.h>

#define ROW (GRID_U + 1)

/* Interseção raio x triângulo (Möller-Trumbore). Devolve a distância t ao
 * longo do raio (origem o, direção d) ou -1 se o raio não acerta. */
static float ray_triangle(Vec3 o, Vec3 d, Vec3 a, Vec3 b, Vec3 c) {
    const float eps = 1e-9f;
    Vec3 e1 = v3_sub(b, a), e2 = v3_sub(c, a);
    Vec3 p = v3_cross(d, e2);
    float det = v3_dot(e1, p), inv, u, v;
    Vec3 s, q;
    if (fabsf(det) < eps) return -1.0f; /* raio paralelo ao triângulo */
    inv = 1.0f / det;
    s = v3_sub(o, a);
    u = v3_dot(s, p) * inv;
    if (u < 0.0f || u > 1.0f) return -1.0f;
    q = v3_cross(s, e1);
    v = v3_dot(d, q) * inv;
    if (v < 0.0f || u + v > 1.0f) return -1.0f;
    return v3_dot(e2, q) * inv;
}

/* Normais pela própria grade: os vizinhos em u e em v dão dois vetores
 * tangentes, e o produto vetorial deles é a normal. Nos polos (onde a
 * linha inteira da grade cai num ponto só) usamos a média da linha vizinha. */
static void grid_normals(const Vec3 *p, Vec3 *n) {
    Vec3 center = v3(0, 0, 0);
    int i, j, k;
    for (k = 0; k < VERTEX_COUNT; ++k) center = v3_add(center, p[k]);
    center = v3_scale(center, 1.0f / VERTEX_COUNT);

    for (j = 0; j <= GRID_V; ++j)
        for (i = 0; i <= GRID_U; ++i) {
            /* A coluna GRID_U é a mesma que a coluna 0 (a grade fecha em u). */
            int left = i == 0 ? GRID_U - 1 : i - 1, right = i == GRID_U ? 1 : i + 1;
            int up = j == 0 ? 0 : j - 1, down = j == GRID_V ? GRID_V : j + 1;
            Vec3 du = v3_sub(p[j * ROW + right], p[j * ROW + left]);
            Vec3 dv = v3_sub(p[down * ROW + i], p[up * ROW + i]);
            Vec3 c = v3_cross(du, dv);
            k = j * ROW + i;
            if (v3_dot(c, c) < 1e-14f) { n[k] = v3(0, 0, 0); continue; } /* polo: depois */
            if (v3_dot(c, v3_sub(p[k], center)) < 0.0f) c = v3_scale(c, -1.0f);
            n[k] = v3_normalize(c);
        }

    for (k = 0; k < VERTEX_COUNT; ++k) {
        if (v3_dot(n[k], n[k]) > 0.5f) continue;
        j = k / ROW;
        {
            int nb = j < GRID_V / 2 ? j + 1 : j - 1; /* linha vizinha, rumo ao equador */
            Vec3 sum = v3(0, 0, 0);
            for (i = 0; i < GRID_U; ++i) sum = v3_add(sum, n[nb * ROW + i]);
            if (v3_dot(sum, sum) < 1e-12f) sum = v3_sub(p[k], center);
            n[k] = v3_normalize(sum);
        }
    }
}

/* Aceleração: cada raio da linha j da grade tem o mesmo ângulo polar
 * phi_j = pi*j/GRID_V. Um triângulo só pode ser atingido pelas linhas cujo
 * phi está perto do phi do triângulo (visto do centro). Então, antes de
 * lançar os raios, guardamos para cada linha a lista dos triângulos que ela
 * pode acertar; cada raio testa só a sua lista, e não o modelo inteiro.
 * As listas ficam todas num vetor só (rows), começando em start[j]. */
typedef struct {
    int start[GRID_V + 2];
    int *rows;
} RowLists;

/* Faixa de linhas [*j0, *j1] que podem enxergar o triângulo t. */
static void triangle_rows(Vec3 a, Vec3 b, Vec3 c, int *j0, int *j1) {
    Vec3 m = v3_scale(v3_add(v3_add(a, b), c), 1.0f / 3.0f); /* já relativo ao centro */
    float r = 0.0f, dist = sqrtf(v3_dot(m, m)), phi, ang;
    Vec3 da = v3_sub(a, m), db = v3_sub(b, m), dc = v3_sub(c, m);
    r = fmaxf(sqrtf(v3_dot(da, da)), fmaxf(sqrtf(v3_dot(db, db)), sqrtf(v3_dot(dc, dc))));
    if (dist <= r * 1.001f) { /* o centro está dentro da esfera do triângulo */
        *j0 = 0;
        *j1 = GRID_V;
        return;
    }
    phi = acosf(clampf(m.y / dist, -1.0f, 1.0f));
    ang = asinf(r / dist) + 1e-3f; /* meio ângulo do cone que contém o triângulo */
    *j0 = (int)floorf((phi - ang) / PI_F * GRID_V);
    *j1 = (int)ceilf((phi + ang) / PI_F * GRID_V);
    if (*j0 < 0) *j0 = 0;
    if (*j1 > GRID_V) *j1 = GRID_V;
}

static int build_row_lists(RowLists *rl, const Vec3 *v, const unsigned *tris, int tcount) {
    int t, j, j0, j1, total = 0;
    int fill[GRID_V + 1];
    memset(rl->start, 0, sizeof(rl->start));
    for (t = 0; t < tcount; ++t) { /* 1ª passada: conta quantos em cada linha */
        triangle_rows(v[tris[3 * t]], v[tris[3 * t + 1]], v[tris[3 * t + 2]], &j0, &j1);
        for (j = j0; j <= j1; ++j) rl->start[j + 1]++;
    }
    for (j = 0; j <= GRID_V; ++j) {
        rl->start[j + 1] += rl->start[j];
        fill[j] = rl->start[j];
    }
    total = rl->start[GRID_V + 1];
    rl->rows = (int *)malloc((size_t)(total > 0 ? total : 1) * sizeof(int));
    if (!rl->rows) return 0;
    for (t = 0; t < tcount; ++t) { /* 2ª passada: preenche */
        triangle_rows(v[tris[3 * t]], v[tris[3 * t + 1]], v[tris[3 * t + 2]], &j0, &j1);
        for (j = j0; j <= j1; ++j) rl->rows[fill[j]++] = t;
    }
    return 1;
}

static int alloc_shape(Shape *out) {
    out->positions = (Vec3 *)malloc(VERTEX_COUNT * sizeof(Vec3));
    out->normals = (Vec3 *)malloc(VERTEX_COUNT * sizeof(Vec3));
    if (out->positions && out->normals) return 1;
    shape_free(out);
    return 0;
}

int shape_from_grid(Shape *out, const Vec3 *positions, const Vec3 *normals) {
    int k;
    if (!alloc_shape(out)) return 0;
    for (k = 0; k < VERTEX_COUNT; ++k) out->positions[k] = positions[k];
    if (normals)
        for (k = 0; k < VERTEX_COUNT; ++k) out->normals[k] = v3_normalize(normals[k]);
    else
        grid_normals(out->positions, out->normals);
    return 1;
}

int shape_fit_mesh(Shape *out, const Vec3 *in_verts, int vcount, const unsigned *tris,
                   int tcount) {
    Vec3 lo, hi, center, *verts;
    RowLists rl;
    float radius = 0.0f;
    int i, j, k;
    const Vec3 origin = v3(0, 0, 0);

    if (vcount <= 0 || tcount <= 0) return -1;

    /* 1) Centro da caixa envolvente e raio: o modelo será levado para
     *    dentro da esfera unitária, do tamanho das formas prontas. */
    lo = hi = in_verts[0];
    for (k = 1; k < vcount; ++k) {
        Vec3 p = in_verts[k];
        lo = v3(fminf(lo.x, p.x), fminf(lo.y, p.y), fminf(lo.z, p.z));
        hi = v3(fmaxf(hi.x, p.x), fmaxf(hi.y, p.y), fmaxf(hi.z, p.z));
    }
    center = v3_scale(v3_add(lo, hi), 0.5f);
    for (k = 0; k < vcount; ++k) {
        Vec3 d = v3_sub(in_verts[k], center);
        radius = fmaxf(radius, sqrtf(v3_dot(d, d)));
    }
    if (radius < 1e-12f) return -1; /* todos os vértices no mesmo ponto */

    /* Cópia dos vértices já centralizada: os raios saem da origem. */
    verts = (Vec3 *)malloc((size_t)vcount * sizeof(Vec3));
    if (!verts) return 0;
    for (k = 0; k < vcount; ++k) verts[k] = v3_sub(in_verts[k], center);
    if (!build_row_lists(&rl, verts, tris, tcount)) {
        free(verts);
        return 0;
    }
    if (!alloc_shape(out)) {
        free(verts);
        free(rl.rows);
        return 0;
    }

    /* 2) Um raio por vértice da grade, na direção da esfera. */
    for (j = 0, k = 0; j <= GRID_V; ++j)
        for (i = 0; i <= GRID_U; ++i, ++k) {
            Vec3 dir = grid_sphere_dir((float)i / GRID_U, (float)j / GRID_V);
            float best = -1.0f;
            int n;
            for (n = rl.start[j]; n < rl.start[j + 1]; ++n) {
                const unsigned *tri = &tris[3 * rl.rows[n]];
                float hit = ray_triangle(origin, dir, verts[tri[0]], verts[tri[1]], verts[tri[2]]);
                if (hit > best) best = hit; /* o ponto mais externo */
            }
            if (best <= 0.0f) {
                /* Nenhum triângulo nessa direção (modelo aberto ou centro
                 * fora dele): usa a distância do vértice mais alinhado. */
                float best_dot = -2.0f;
                int v;
                for (v = 0; v < vcount; ++v) {
                    Vec3 d = verts[v];
                    float len = sqrtf(v3_dot(d, d)), dd;
                    if (len < 1e-12f) continue;
                    dd = v3_dot(d, dir) / len;
                    if (dd > best_dot) { best_dot = dd; best = len; }
                }
                if (best < 0.0f) best = 0.0f;
            }
            out->positions[k] = v3_scale(dir, best / radius);
        }

    free(verts);
    free(rl.rows);

    /* 3) Normais calculadas na própria grade. */
    grid_normals(out->positions, out->normals);
    return 1;
}
