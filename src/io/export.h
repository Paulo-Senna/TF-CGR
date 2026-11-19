/* export.h - Exporta a forma exibida (mistura + deformações) como modelo 3D.
 *
 * Gera, na pasta escolhida:
 *   nome.obj           a malha deformada (posições, normais e coordenadas de textura)
 *   nome.mtl           o material (cor da forma)
 *   nome_textura.xxx   a textura em uso, se houver (.tga para as geradas
 *                      pelo programa; cópia do arquivo para uma imagem carregada)
 * A rotação automática da cena não entra: o modelo sai na posição original. */
#ifndef EXPORT_H
#define EXPORT_H

/* 'obj_path' ganha a extensão .obj se não tiver. Resultado em app.export_msg.
 * Retorna 1 se deu certo. */
int export_model(const char *obj_path);

#endif /* EXPORT_H */
