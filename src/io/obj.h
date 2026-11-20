/* obj.h - Leitura e escrita de modelos 3D no formato Wavefront OBJ.
 *
 * O OBJ é texto puro e é aberto por praticamente todo programa 3D
 * (Blender, MeshLab, visualizadores de sistema...). Linhas usadas aqui:
 *   v  x y z        posição de um vértice
 *   vn x y z        normal
 *   f  v//n ...     face (índices começando em 1)
 * O resto (grupos, materiais, curvas...) é ignorado na leitura. */
#ifndef OBJ_H
#define OBJ_H

#include <stddef.h>

#include "geometry/mathutil.h"

/* Comentário que marca um arquivo salvo por este programa: os vértices
 * já estão na ordem da grade, então dá para reabri-lo sem reamostrar. */
#define OBJ_GRID_MARK "# morph3d-grade"

typedef struct {
    Vec3 *positions; /* linhas 'v' */
    int vcount;
    Vec3 *normals; /* linhas 'vn' (podem não existir) */
    int ncount;
    unsigned *tris; /* 3 índices (base 0) por triângulo; polígonos viram leques */
    int tcount;
    int grid_file; /* 1 se o arquivo tem a marca OBJ_GRID_MARK */
} ObjMesh;

/* Lê o arquivo. Em caso de erro retorna 0 e escreve a mensagem em 'err'. */
int obj_read(const char *path, ObjMesh *mesh, char *err, size_t err_size);
void obj_free(ObjMesh *mesh);

/* Escreve vértices (posição e normal) e triângulos. 'header' entra como
 * comentário no topo; 'mtl_file' e 'material' podem ser NULL. Retorna 0 se
 * não conseguir criar o arquivo. */
int obj_write(const char *path, const char *header, const char *mtl_file, const char *material,
              const Vec3 *pos, const Vec3 *nrm, int vcount,
              const unsigned *idx, int icount);

#endif /* OBJ_H */
