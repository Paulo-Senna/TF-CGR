/* morph.c - Interpolação linear entre duas formas (ver morph.h). */
#include "geometry/morph.h"

/* Interpolação linear de um vetor: (1 - t) * a + t * b.
 * Escrita por extenso para deixar a fórmula do morph bem visível. */
static Vec3 lerp(Vec3 a, Vec3 b, float t) {
    Vec3 r;
    r.x = (1.0f - t) * a.x + t * b.x;
    r.y = (1.0f - t) * a.y + t * b.y;
    r.z = (1.0f - t) * a.z + t * b.z;
    return r;
}

void morph_blend(const Shape *a, const Shape *b, float t, Vec3 *out_pos, Vec3 *out_nrm) {
    int i;
    /* Mesmo índice i nas duas formas = pontos correspondentes (mesma grade). */
    for (i = 0; i < VERTEX_COUNT; ++i) {
        /* Posição: anda em linha reta do ponto de A até o ponto de B. */
        out_pos[i] = lerp(a->positions[i], b->positions[i], t);

        /* Normal (usada na iluminação): também é interpolada, mas depois
         * precisa voltar a ter comprimento 1. Se as duas normais forem
         * opostas, no meio do caminho elas se anulam; v3_normalize trata
         * esse caso devolvendo (0,1,0) em vez de dividir por zero. */
        out_nrm[i] = v3_normalize(lerp(a->normals[i], b->normals[i], t));
    }
}

Vec3 morph_color(const Shape *a, const Shape *b, float t) {
    return lerp(a->color, b->color, t);
}
