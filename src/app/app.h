/* app.h - Estado global do TF-CGR e operações sobre ele.
 *
 * O estado fica numa única struct (app), compartilhada por main.c
 * (janela, entrada, renderização) e ui.c (painel). As funções app_*
 * concentram a lógica do morph para que teclado e painel façam
 * exatamente a mesma coisa. */
#ifndef APP_H
#define APP_H

#include <glad/gl.h>

#include "geometry/deform.h"
#include "geometry/shapes.h"
#include "render/texture.h"

/* ----------------------------------------------------------------------------
 * Parâmetros da animação e da câmera
 * ------------------------------------------------------------------------- */
#define MORPH_DURATION 2.5f /* segundos para ir de uma forma à outra */
#define HOLD_TIME 1.0f      /* pausa em cada forma no modo automático */
#define MANUAL_SPEED 0.5f   /* velocidade de t com as setas (por segundo) */

#define CAM_YAW 0.6f /* câmera inicial */
#define CAM_PITCH 0.35f
#define CAM_DIST 4.0f
#define CAM_DIST_MIN 1.5f
#define CAM_DIST_MAX 15.0f

/* ----------------------------------------------------------------------------
 * Estado
 * ------------------------------------------------------------------------- */
typedef struct {
    int from;   /* forma A (origem) */
    int to;     /* forma B (destino) */
    float t;    /* progresso da mistura em [0,1] (0 = forma A, 1 = forma B) */
    float hold; /* tempo restante de pausa na forma atual (modo automático) */
    int auto_play;
    int queued; /* destino escolhido pelo usuário durante um morph (-1 = nenhum) */
} MorphState;

typedef struct {
    float yaw, pitch, dist;
    int dragging;
    double last_x, last_y;
} Camera;

/* As deformações de Barr (struct Deform) estão em geometry/deform.h. */

/* Mensagem curta mostrada no painel (verde = deu certo, vermelho = erro). */
typedef struct {
    char text[256];
    int is_error;
} Message;

/* Textura aplicada sobre a forma. */
#define TEX_SCALE_MIN 0.5f
#define TEX_SCALE_MAX 8.0f

typedef struct {
    int current;         /* TEX_NONE, TEX_CHECKER, ... */
    GLuint ids[TEX_COUNT];
    float scale;         /* "Repetição": multiplica o (u,v) */
    int multiply;        /* multiplica a textura pela cor da forma */
} TexState;

enum { RENDER_SOLID = 0, RENDER_SOLID_WIRE, RENDER_WIRE, RENDER_MODE_COUNT };

/* Posições dos uniforms, consultadas uma única vez após linkar o programa. */
typedef struct {
    GLint model, view, proj, t, color, light_pos, eye_pos, wireframe, wire_color;
    GLint taper, twist, bend;
    GLint tex, use_texture, tex_multiply, tex_scale;
} Uniforms;

typedef struct {
    /* Geometria (CPU e GPU) */
    Shape shapes[MAX_SHAPES]; /* as prontas e depois as adicionadas (.obj) */
    int shape_count;
    GLuint vbos[MAX_SHAPES];  /* um VBO por forma (posição + normal intercaladas) */
    GLuint uv_vbo;            /* coordenadas (u,v) da grade, compartilhadas */
    GLuint vao, ebo, program;
    Uniforms loc;

    /* Estado interativo */
    MorphState morph;
    Deform deform;
    TexState tex;
    Camera cam;
    int render_mode;
    int rotate;
    float obj_angle;
    Message shape_msg;  /* resultado de "Adicionar forma" */
    Message export_msg; /* resultado de "Exportar" */
} App;

extern App app;

/* ----------------------------------------------------------------------------
 * Operações (app.c)
 * ------------------------------------------------------------------------- */
void app_reset_state(void);  /* estado inicial (mantém as formas adicionadas) */
void app_reset_camera(void);
void app_reset_deform(void);
void app_reset_texture(void); /* volta a "Nenhuma" */


/* Lê um modelo .obj, encaixa na grade (geometry/mesh_fit.h), envia para a
 * GPU e já mostra: a nova forma vira a forma A, parada (t = 0), pronta
 * para ser deformada. Retorna 1 se deu certo (resultado em shape_msg). */
int app_add_shape_file(const char *path);

/* Envia a forma 'index' para o seu VBO (cria o VBO se preciso). */
void app_upload_shape(int index);

/* Mostra o resultado de uma operação no painel e no terminal. */
void app_message(Message *m, int is_error, const char *text);


void app_bind_shape_pair(void);             /* religa A/B nos atributos do VAO */
void app_set_shapes(int from, int to);      /* escolha direta de A e B (painel) */
void app_choose_target(int target);         /* teclas 1..9 */
void app_set_t(float t);                    /* slider: posiciona o morph e pausa */
void app_update(float dt, int manual_dir);  /* manual_dir: -1, 0 ou +1 (setas) */

#endif /* APP_H */
