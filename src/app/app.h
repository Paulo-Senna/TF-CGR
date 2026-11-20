/* app.h - Estado do programa e as ações de cada tecla.
 *
 * Todo o estado fica numa única struct global (app). O main.c lê o
 * teclado/mouse e chama as funções app_*; o render.c lê o estado para
 * desenhar. Tudo que o usuário precisa ver é escrito no terminal. */
#ifndef APP_H
#define APP_H

#include "geometry/shapes.h"

/* ----------------------------------------------------------------------------
 * Parâmetros
 * ------------------------------------------------------------------------- */
#define MORPH_SPEED 0.4f   /* quanto t anda por segundo segurando <- ou -> */
#define ZOOM_SPEED 3.0f    /* unidades por segundo segurando , ou . */
#define ROTATE_SPEED 0.4f  /* radianos por segundo (tecla R) */

#define CAM_YAW 0.6f       /* câmera inicial */
#define CAM_PITCH 0.35f
#define CAM_DIST 4.0f
#define CAM_DIST_MIN 1.5f  /* zoom máximo */
#define CAM_DIST_MAX 15.0f /* zoom mínimo */

/* Etapas da escolha de formas com a tecla Q. */
enum { CHOOSE_OFF = 0, CHOOSE_A, CHOOSE_B };

/* ----------------------------------------------------------------------------
 * Estado
 * ------------------------------------------------------------------------- */
typedef struct {
    float yaw, pitch, dist; /* câmera orbital (ângulos e distância) */
    int dragging;           /* botão esquerdo pressionado? */
    double last_x, last_y;  /* última posição do mouse */
} Camera;

typedef struct {
    /* Formas: as 5 prontas e depois as importadas (.obj), até 9. */
    Shape shapes[MAX_SHAPES];
    int shape_count;

    /* Morph */
    int shape_a; /* forma inicial */
    int shape_b; /* forma final */
    float t;     /* progresso: 0 = só A, 1 = só B */

    /* Escolha de formas (Q, depois um número para A e outro para B) */
    int choosing; /* CHOOSE_OFF, CHOOSE_A ou CHOOSE_B */
    int chosen_a; /* forma A já escolhida (na etapa CHOOSE_B) */

    /* Visualização */
    Camera cam;
    int wireframe;   /* 0 = sólido, 1 = só malha (tecla W) */
    int rotate;      /* rotação automática ligada? (tecla R) */
    float obj_angle; /* ângulo atual da rotação */
} App;

extern App app;

/* ----------------------------------------------------------------------------
 * Funções
 * ------------------------------------------------------------------------- */
int app_init(void); /* cria as formas prontas; retorna 0 se faltar memória */
void app_shutdown(void);

/* Avança o tempo: morph_dir e zoom_dir valem -1, 0 ou +1 (teclas seguradas). */
void app_update(float dt, int morph_dir, int zoom_dir);

/* Ações das teclas */
void app_start_choosing(void);   /* Q */
void app_number_key(int number); /* 1..9 (number começa em 0) */
void app_toggle_wireframe(void); /* W */
void app_toggle_rotation(void);  /* R */
void app_import_dialog(void);    /* I: abre a janela para escolher um .obj */
void app_export_dialog(void);    /* O: abre a janela para salvar um .obj */

/* Importa/exporta sem janela (arrastar arquivo e linha de comando). */
int app_import(const char *path);
int app_export(const char *path);

void app_print_help(void);
/* printf que não atrapalha a barra de progresso do morph no terminal. */
void app_print(const char *fmt, ...);

#endif /* APP_H */
