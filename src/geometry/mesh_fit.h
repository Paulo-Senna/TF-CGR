/* mesh_fit.h - Transforma um modelo 3D qualquer (arquivo .obj) numa forma
 * sobre a grade paramétrica, para ele poder participar do morph.
 *
 * O morph precisa de correspondência 1:1 entre os vértices de todas as
 * formas, mas um modelo importado tem a SUA malha (outro número de
 * vértices, outra ordem). A solução é REAMOSTRAR o modelo na nossa grade:
 * para cada vértice (u, v) da esfera, lançamos um raio do centro do
 * modelo na direção da esfera e pegamos o ponto mais externo em que o
 * raio fura a superfície. Assim o vértice (u, v) da esfera vira o ponto
 * do modelo "na mesma direção", e o morph Esfera -> modelo fica natural.
 *
 * Limitação: partes escondidas da vista do centro (cavidades, uma alça
 * de xícara) não aparecem, porque cada direção guarda só um ponto. */
#ifndef MESH_FIT_H
#define MESH_FIT_H

#include "geometry/shapes.h"

/* Reamostra a malha (verts[vcount], triângulos tris[3*tcount]) na grade.
 * O modelo é centralizado e escalado para caber numa esfera de raio 1.
 * Aloca out->positions/normals. Retorna 1 se deu certo, 0 se faltar
 * memória e -1 se a malha for vazia ou degenerada (tudo num ponto). */
int shape_fit_mesh(Shape *out, const Vec3 *verts, int vcount, const unsigned *tris, int tcount);

/* Usa diretamente VERTEX_COUNT posições que JÁ estão na ordem da grade
 * (um arquivo exportado por este programa). 'normals' pode ser NULL
 * (então as normais são calculadas pela grade). Retorna 0 se faltar memória. */
int shape_from_grid(Shape *out, const Vec3 *positions, const Vec3 *normals);

#endif /* MESH_FIT_H */
