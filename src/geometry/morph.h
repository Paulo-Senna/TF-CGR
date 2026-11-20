/* morph.h - O MORPH: transforma uma forma A numa forma B.
 *
 * Esta é a parte central do trabalho, feita à mão em C (sem biblioteca).
 *
 * O problema do morph 3D tem duas etapas (Alexa, 2002):
 *   1. CORRESPONDÊNCIA: decidir qual ponto de A vira qual ponto de B.
 *   2. INTERPOLAÇÃO: definir o caminho de cada ponto de A até o seu par em B.
 *
 * Etapa 1 - resolvida na criação das formas (shapes.c e mesh_fit.c):
 *   toda forma é amostrada na MESMA grade (u, v), com o mesmo número de
 *   vértices e na mesma ordem. Então o vértice i de A corresponde ao
 *   vértice i de B. Modelos .obj são projetados nessa grade a partir do
 *   centro, como na projeção esférica de Kent, Carlson e Parent (1992).
 *
 * Etapa 2 - feita aqui: interpolação linear, vértice a vértice,
 *       P(t) = (1 - t) * A + t * B        com t em [0, 1]
 *   t = 0 mostra A, t = 1 mostra B e qualquer valor no meio é uma forma
 *   intermediária. Por isso dá para parar a transformação em qualquer ponto. */
#ifndef MORPH_H
#define MORPH_H

#include "geometry/shapes.h"

/* Calcula a forma intermediária entre 'a' e 'b' no instante t.
 * 'out_pos' e 'out_nrm' precisam ter VERTEX_COUNT posições. */
void morph_blend(const Shape *a, const Shape *b, float t, Vec3 *out_pos, Vec3 *out_nrm);

/* Cor intermediária (a cor também "se transforma" de A para B). */
Vec3 morph_color(const Shape *a, const Shape *b, float t);

#endif /* MORPH_H */
