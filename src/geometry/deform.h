/* deform.h - Deformações de Barr (taper, twist, bend) calculadas na CPU.
 *
 * Na tela quem deforma é o vertex shader (shaders/morph.vert). Estas
 * funções fazem EXATAMENTE a mesma conta em C, para a exportação do
 * modelo 3D sair igual ao que aparece na janela. Se mudar uma, mude a outra. */
#ifndef DEFORM_H
#define DEFORM_H

#include "geometry/mathutil.h"

#define TAPER_MIN -0.9f
#define TAPER_MAX 0.9f
#define TWIST_MIN -6.0f
#define TWIST_MAX 6.0f
#define BEND_MIN -1.5f
#define BEND_MAX 1.5f

typedef struct {
    float taper; /* afunilamento: x,z *= 1 + taper*y */
    float twist; /* torção: radianos por unidade de y */
    float bend;  /* dobra: curvatura (1/raio) */
} Deform;

/* Posição deformada (ordem: taper -> twist -> bend, ao longo de Y). */
Vec3 deform_point(Vec3 p, const Deform *d);

/* Normal da superfície deformada no ponto p (normal original n),
 * por diferenças finitas, como no shader. */
Vec3 deform_normal(Vec3 p, Vec3 n, const Deform *d);

#endif /* DEFORM_H */
