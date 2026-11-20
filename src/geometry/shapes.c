/* shapes.c - Formas paramétricas sobre a mesma grade (u, v).
 *
 * Convenção da grade, para todas as formas:
 *   u em [0,1] -> ângulo theta = 2*pi*u em torno do eixo Y
 *   v em [0,1] -> vai do "topo" (v=0) até a "base" (v=1)
 * Pontos com o mesmo (u, v) em formas diferentes são correspondentes. */
#include "geometry/shapes.h"

#include <stdio.h>
#include <stdlib.h>

/* Uma função por forma: recebe (u, v) e devolve posição e normal. */
typedef void (*ShapeFn)(float u, float v, Vec3 *pos, Vec3 *nrm);

/* Direção na esfera unitária para o par (u, v). */
Vec3 grid_sphere_dir(float u, float v) {
    float theta = 2.0f * PI_F * u;
    float phi = PI_F * v; /* 0 = polo norte, pi = polo sul */
    return v3(sinf(phi) * cosf(theta), cosf(phi), sinf(phi) * sinf(theta));
}

static void sphere(float u, float v, Vec3 *p, Vec3 *n) {
    *n = grid_sphere_dir(u, v);
    *p = *n; /* raio 1 */
}

/* Faixa de v ocupada por cada tampa (cubo e cilindro). Com GRID_V
 * múltiplo de 4, a borda das tampas cai exatamente numa linha da grade. */
#define CAP 0.25f

/* Cubo: mesma estrutura do cilindro (tampa / lateral / tampa), mas o
 * contorno é um quadrado em vez de um círculo. A direção (cos, sin) é
 * projetada no quadrado; os cantos ficam em theta = 45°, 135°... que
 * coincidem com colunas da grade (GRID_U múltiplo de 8). Assim as
 * arestas do cubo ficam alinhadas com as arestas da malha (sem serrilhado). */
static void cube(float u, float v, Vec3 *p, Vec3 *n) {
    const float h = 0.8f; /* meia aresta */
    const float eps = 1e-4f;
    float theta = 2.0f * PI_F * u;
    float c = cosf(theta), s = sinf(theta);
    float k = h / maxf(fabsf(c), fabsf(s)); /* raio até o quadrado */
    if (v < CAP) {
        float r = k * (v / CAP);
        *p = v3(r * c, h, r * s);
    } else if (v > 1.0f - CAP) {
        float r = k * ((1.0f - v) / CAP);
        *p = v3(r * c, -h, r * s);
    } else {
        float t = (v - CAP) / (1.0f - 2.0f * CAP);
        *p = v3(k * c, h - 2.0f * h * t, k * s);
    }
    /* Normal: soma das normais das faces em que o ponto está. Nas arestas
     * e cantos isso dá a média das faces vizinhas. */
    *n = v3(fabsf(p->x) > h - eps ? (p->x > 0 ? 1.0f : -1.0f) : 0.0f,
            fabsf(p->y) > h - eps ? (p->y > 0 ? 1.0f : -1.0f) : 0.0f,
            fabsf(p->z) > h - eps ? (p->z > 0 ? 1.0f : -1.0f) : 0.0f);
}

/* Toro: theta percorre o anel maior, phi percorre o tubo.
 * phi = pi - 2*pi*v faz o "equador" da esfera (v=0.5) cair na borda
 * externa do toro e os polos (v=0 e v=1) caírem na borda interna. */
static void torus(float u, float v, Vec3 *p, Vec3 *n) {
    const float R = 0.75f, r = 0.32f;
    float theta = 2.0f * PI_F * u;
    float phi = PI_F - 2.0f * PI_F * v;
    float ring = R + r * cosf(phi);
    *p = v3(ring * cosf(theta), r * sinf(phi), ring * sinf(theta));
    *n = v3(cosf(phi) * cosf(theta), sinf(phi), cosf(phi) * sinf(theta));
}

/* Cilindro com tampas: a faixa v é dividida em tampa superior,
 * lateral e tampa inferior. */
static void cylinder(float u, float v, Vec3 *p, Vec3 *n) {
    const float radius = 0.75f, h = 0.9f;
    float theta = 2.0f * PI_F * u;
    float c = cosf(theta), s = sinf(theta);
    if (v < CAP) { /* tampa de cima */
        float k = radius * (v / CAP);
        *p = v3(k * c, h, k * s);
        *n = v3(0.0f, 1.0f, 0.0f);
    } else if (v > 1.0f - CAP) { /* tampa de baixo */
        float k = radius * ((1.0f - v) / CAP);
        *p = v3(k * c, -h, k * s);
        *n = v3(0.0f, -1.0f, 0.0f);
    } else { /* lateral */
        float t = (v - CAP) / (1.0f - 2.0f * CAP);
        *p = v3(radius * c, h - 2.0f * h * t, radius * s);
        *n = v3(c, 0.0f, s);
    }
}

/* Plano: quadrado de lado 2 no plano XZ (y = 0), virado para cima.
 * É a parametrização mais direta possível: u anda em X e v anda em Z.
 * Não tem "volume", então o morph Plano -> forma fechada mostra a grade
 * se dobrando para formar a superfície (ótimo para ver a correspondência). */
static void plane(float u, float v, Vec3 *p, Vec3 *n) {
    *p = v3((u - 0.5f) * 2.0f, 0.0f, (v - 0.5f) * 2.0f);
    *n = v3(0.0f, 1.0f, 0.0f);
}

/* Amostra a função 'fn' em todos os vértices da grade. */
static int sample(Shape *s, const char *name, ShapeFn fn, Vec3 color) {
    int i, j, k = 0;
    snprintf(s->name, sizeof(s->name), "%s", name);
    s->color = color;
    s->positions = (Vec3 *)malloc(VERTEX_COUNT * sizeof(Vec3));
    s->normals = (Vec3 *)malloc(VERTEX_COUNT * sizeof(Vec3));
    if (!s->positions || !s->normals) return 0;
    for (j = 0; j <= GRID_V; ++j) {
        for (i = 0; i <= GRID_U; ++i, ++k) {
            Vec3 p, n;
            fn((float)i / GRID_U, (float)j / GRID_V, &p, &n);
            s->positions[k] = p;
            s->normals[k] = v3_normalize(n);
        }
    }
    return 1;
}

int shapes_build(Shape *shapes) {
    int ok = 1;
    ok &= sample(&shapes[SHAPE_SPHERE],   "Esfera",   sphere,   v3(0.25f, 0.55f, 0.95f));
    ok &= sample(&shapes[SHAPE_CUBE],     "Cubo",     cube,     v3(0.95f, 0.55f, 0.20f));
    ok &= sample(&shapes[SHAPE_TORUS],    "Toro",     torus,    v3(0.30f, 0.80f, 0.40f));
    ok &= sample(&shapes[SHAPE_CYLINDER], "Cilindro", cylinder, v3(0.70f, 0.40f, 0.90f));
    ok &= sample(&shapes[SHAPE_PLANE],    "Plano",    plane,    v3(0.90f, 0.78f, 0.30f));
    return ok;
}

void shape_free(Shape *s) {
    free(s->positions);
    free(s->normals);
    s->positions = s->normals = NULL;
}

void grid_build_indices(unsigned int *out) {
    const unsigned int row = GRID_U + 1;
    unsigned int i, j;
    for (j = 0; j < GRID_V; ++j) {
        for (i = 0; i < GRID_U; ++i) {
            unsigned int a = j * row + i, b = a + 1;
            unsigned int c = a + row, d = c + 1;
            *out++ = a; *out++ = c; *out++ = b;
            *out++ = b; *out++ = c; *out++ = d;
        }
    }
}
