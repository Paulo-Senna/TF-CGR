/* shapes.h - Geração das formas usadas no morphing.
 *
 * Ideia central: TODAS as formas são amostradas sobre a MESMA grade
 * paramétrica (u, v) com a mesma resolução. Assim o vértice i da esfera
 * corresponde ao vértice i do cubo, do toro etc. (correspondência 1:1),
 * e o morphing vira uma simples interpolação vértice a vértice. */
#ifndef SHAPES_H
#define SHAPES_H

#include "geometry/mathutil.h"

/* Resolução da grade paramétrica (igual para todas as formas). */
#define GRID_U 64 /* divisões em torno do eixo Y (longitude) */
#define GRID_V 48 /* divisões de cima para baixo (latitude) */

#define VERTEX_COUNT ((GRID_U + 1) * (GRID_V + 1))
#define INDEX_COUNT (GRID_U * GRID_V * 6)

/* "static_assert" de C99: o array teria tamanho negativo se a condição
 * falhasse. As arestas do cubo/cilindro precisam cair em linhas da grade. */
typedef char grid_alinhada_com_arestas[(GRID_U % 8 == 0 && GRID_V % 4 == 0) ? 1 : -1];

/* Formas prontas (a ordem é a das teclas 1, 2, 3...). As formas
 * adicionadas pelo usuário (arquivos .obj) entram depois delas. */
enum {
    SHAPE_SPHERE = 0,
    SHAPE_CUBE,
    SHAPE_TORUS,
    SHAPE_CYLINDER,
    SHAPE_PLANE,
    SHAPE_BUILTIN_COUNT
};

#define MAX_SHAPES 9 /* teclas 1 a 9 */

typedef struct {
    char name[32];
    Vec3 *positions; /* VERTEX_COUNT posições, uma por vértice da grade */
    Vec3 *normals;   /* VERTEX_COUNT normais */
    Vec3 color;      /* cor base da forma */
} Shape;

/* Preenche shapes[0..SHAPE_BUILTIN_COUNT-1]. Retorna 0 se faltar memória. */
int shapes_build(Shape *shapes);
void shape_free(Shape *s);

/* Direção na esfera unitária para o par (u, v) da grade. */
Vec3 grid_sphere_dir(float u, float v);

/* Índices dos triângulos da grade (compartilhados por todas as formas).
 * 'out' precisa ter INDEX_COUNT posições. */
void grid_build_indices(unsigned int *out);

#endif /* SHAPES_H */
