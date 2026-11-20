/* export.h - Salva a forma que está na tela (a mistura A/B no t atual).
 *
 * Gera, na pasta escolhida:
 *   nome.obj   a malha (posições e normais)
 *   nome.mtl   o material (cor da forma)
 * A rotação da cena não entra: o modelo sai na posição original. */
#ifndef EXPORT_H
#define EXPORT_H

#include <stddef.h>

#include "geometry/shapes.h"

/* 'obj_path' ganha a extensão .obj se não tiver. Escreve o resultado
 * (sucesso ou erro) em 'msg'. Retorna 1 se deu certo. */
int export_model(const char *obj_path, const Shape *a, const Shape *b, float t, char *msg,
                 size_t msg_size);

#endif /* EXPORT_H */
