/* app.h - Estado global do Laboratório de Deformação e operações sobre ele.
 *
 * O estado fica numa única struct (app), compartilhada por main.c
 * (janela, entrada, renderização) e ui.c (painel). As funções app_*
 * concentram a lógica do morph para que teclado e painel façam
 * exatamente a mesma coisa. */
#ifndef APP_H
#define APP_H

#include <glad/gl.h>

#include "shapes.h"
#include "texture.h"

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
    float t;    /* progresso LINEAR em [0,1]; a forma usa eased(t) */
    float hold; /* tempo restante de pausa na forma atual (modo automático) */
    int auto_play;
    int smooth; /* usa curva de suavização (smoothstep) */
    int queued; /* destino escolhido pelo usuário durante um morph (-1 = nenhum) */
} MorphState;

typedef struct {
    float yaw, pitch, dist;
    int dragging;
    double last_x, last_y;
} Camera;

/* Deformações de Barr aplicadas depois da mistura (ver morph.vert). */
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

/* Textura aplicada sobre a forma. */
#define TEX_SCALE_MIN 0.5f
#define TEX_SCALE_MAX 8.0f

typedef struct {
    int current;         /* TEX_NONE, TEX_CHECKER, ... */
    GLuint ids[TEX_COUNT];
    float scale;         /* "Repetição": multiplica o (u,v) */
    int multiply;        /* multiplica a textura pela cor da forma */
    char path[512];      /* campo de texto do painel */
    char image_info[96]; /* nome/tamanho da imagem carregada */
    char message[256];   /* última mensagem (sucesso ou erro) */
    int message_is_error;
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
    Shape shapes[SHAPE_COUNT];
    GLuint vbos[SHAPE_COUNT]; /* um VBO por forma (posição + normal intercaladas) */
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
} App;

extern App app;

/* ----------------------------------------------------------------------------
 * Operações (app.c)
 * ------------------------------------------------------------------------- */
void app_reset_state(void);  /* estado inicial (sem mexer em recursos da GPU) */
void app_reset_camera(void);
void app_reset_deform(void);
void app_reset_texture(void); /* volta a "Nenhuma" (mantém a imagem carregada) */

/* Carrega um PNG/JPG e passa a usá-lo. Em caso de erro, mostra a mensagem
 * no painel e no terminal e mantém a textura atual. Retorna 1 se carregou. */
int app_load_image(const char *path);

float app_eased_t(void); /* t depois da curva (linear ou smoothstep) */

void app_bind_shape_pair(void);             /* religa A/B nos atributos do VAO */
void app_set_shapes(int from, int to);      /* escolha direta de A e B (painel) */
void app_choose_target(int target);         /* teclas 1..5 */
void app_set_t(float t);                    /* slider: posiciona o morph e pausa */
void app_set_smooth(int smooth);            /* troca a curva SEM salto na forma */
void app_update(float dt, int manual_dir);  /* manual_dir: -1, 0 ou +1 (setas) */

#endif /* APP_H */
