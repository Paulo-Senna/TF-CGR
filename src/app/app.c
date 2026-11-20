/* app.c - Estado do programa e ações das teclas (ver app.h). */
#include "app/app.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "geometry/mesh_fit.h"
#include "io/export.h"
#include "io/file_dialog.h"
#include "io/obj.h"

App app;

/* A barra de progresso do morph é redesenhada na mesma linha do terminal
 * (com '\r'). Antes de qualquer outra mensagem é preciso pular de linha. */
static int progress_on_screen = 0;
static int last_percent = -1;

/* ============================================================================
 * Mensagens no terminal
 * ========================================================================== */
void app_print(const char *fmt, ...) {
    va_list args;
    if (progress_on_screen) {
        printf("\n");
        progress_on_screen = 0;
    }
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
    fflush(stdout);
}

/* Barra de progresso, ex.:  Esfera [##########----------] Cubo  50% */
static void print_progress(void) {
    const int width = 20;
    int percent = (int)(app.t * 100.0f + 0.5f);
    int filled = (int)(app.t * width + 0.5f), i;
    char bar[32];
    for (i = 0; i < width; ++i) bar[i] = i < filled ? '#' : '-';
    bar[width] = '\0';
    printf("\r  %s [%s] %s  %3d%%   ", app.shapes[app.shape_a].name, bar,
           app.shapes[app.shape_b].name, percent);
    fflush(stdout);
    progress_on_screen = 1;
    last_percent = percent;
}

static void print_shape_list(void) {
    int i;
    char line[512] = "  Formas:";
    for (i = 0; i < app.shape_count; ++i) {
        size_t n = strlen(line);
        snprintf(line + n, sizeof(line) - n, "  [%d] %s", i + 1, app.shapes[i].name);
    }
    app_print("%s", line);
}

void app_print_help(void) {
    app_print("\n=== TF-CGR - Morph de objetos 3D ===\n"
              "  H         help\n"
              "  <- / ->   A em B\n"
              "  Q         redefinir formas\n"
              "  , / .     zoom\n"
              "  W         malha\n"
              "  R         rotação\n"
              "  I         input\n"
              "  O         output\n"
              "  ESC       sair");
    print_shape_list();
    print_progress();
}

/* ============================================================================
 * Início e fim
 * ========================================================================== */
int app_init(void) {
    memset(&app, 0, sizeof(app));
    app.shape_count = SHAPE_BUILTIN_COUNT;
    if (!shapes_build(app.shapes)) return 0;

    app.shape_a = SHAPE_SPHERE;
    app.shape_b = SHAPE_CUBE;
    app.t = 0.0f;
    app.choosing = CHOOSE_OFF;

    app.cam.yaw = CAM_YAW;
    app.cam.pitch = CAM_PITCH;
    app.cam.dist = CAM_DIST;
    app.wireframe = 0;
    app.rotate = 1;
    return 1;
}

void app_shutdown(void) {
    int i;
    for (i = 0; i < app.shape_count; ++i) shape_free(&app.shapes[i]);
}

/* ============================================================================
 * Atualização a cada quadro
 * ========================================================================== */
void app_update(float dt, int morph_dir, int zoom_dir) {
    /* Setas: t anda para frente (->) ou para trás (<-) enquanto seguradas.
     * Soltando a tecla, o morph para exatamente onde está. */
    if (morph_dir != 0) {
        app.t = clampf(app.t + morph_dir * MORPH_SPEED * dt, 0.0f, 1.0f);
        if ((int)(app.t * 100.0f + 0.5f) != last_percent) print_progress();
    }

    /* Vírgula aproxima (distância menor), ponto afasta. */
    app.cam.dist = clampf(app.cam.dist + zoom_dir * ZOOM_SPEED * dt, CAM_DIST_MIN, CAM_DIST_MAX);

    if (app.rotate) app.obj_angle += ROTATE_SPEED * dt;
}

/* ============================================================================
 * Escolha das formas: Q -> número da forma A -> número da forma B
 * ========================================================================== */
void app_start_choosing(void) {
    app.choosing = CHOOSE_A;
    app_print("Escolha a forma INICIAL (A): tecla 1 a %d", app.shape_count);
    print_shape_list();
}

void app_number_key(int number) {
    if (app.choosing == CHOOSE_OFF) {
        app_print("Aperte Q para escolher as formas (primeiro A, depois B).");
        return;
    }
    if (number < 0 || number >= app.shape_count) {
        app_print("Nao existe a forma %d (sao %d formas).", number + 1, app.shape_count);
        return;
    }

    if (app.choosing == CHOOSE_A) {
        app.chosen_a = number;
        app.choosing = CHOOSE_B;
        app_print("A = %s. Agora escolha a forma FINAL (B).", app.shapes[number].name);
        return;
    }

    /* Etapa B: precisa ser diferente de A (de A para A nada mudaria). */
    if (number == app.chosen_a) {
        app_print("A forma B precisa ser diferente de A (%s). Escolha outra.",
                  app.shapes[number].name);
        return;
    }
    app.shape_a = app.chosen_a;
    app.shape_b = number;
    app.t = 0.0f; /* recomeça do inicio: mostrando a forma A */
    app.choosing = CHOOSE_OFF;
    app_print("Novo morph: %s -> %s. Segure -> para transformar.", app.shapes[app.shape_a].name,
              app.shapes[app.shape_b].name);
    print_progress();
}

/* ============================================================================
 * Visualização
 * ========================================================================== */
void app_toggle_wireframe(void) {
    app.wireframe = !app.wireframe; /* o resultado já aparece na tela */
}

void app_toggle_rotation(void) {
    app.rotate = !app.rotate; /* o resultado já aparece na tela */
}

/* ============================================================================
 * Importar e exportar .obj
 * ========================================================================== */

/* Só o nome do arquivo, sem as pastas e sem a extensão. */
static void file_title(const char *path, char *out, size_t size) {
    const char *a = strrchr(path, '/'), *b = strrchr(path, '\\');
    size_t len;
    if (b > a) a = b;
    snprintf(out, size, "%s", a ? a + 1 : path);
    len = strlen(out);
    if (len > 4 && out[len - 4] == '.') out[len - 4] = '\0';
}

/* Cores das formas importadas (uma por vez, em ciclo). */
static Vec3 imported_color(int n) {
    static const float colors[][3] = {
        {0.90f, 0.35f, 0.45f}, {0.25f, 0.75f, 0.80f}, {0.85f, 0.85f, 0.85f}, {0.95f, 0.65f, 0.35f}};
    const float *c = colors[n % 4];
    return v3(c[0], c[1], c[2]);
}

int app_import(const char *path) {
    ObjMesh mesh;
    Shape *sh;
    char err[256];
    int index = app.shape_count, ok;

    if (index >= MAX_SHAPES) {
        app_print("Limite de %d formas atingido (teclas 1 a 9).", MAX_SHAPES);
        return 0;
    }
    if (!obj_read(path, &mesh, err, sizeof(err))) {
        app_print("%s", err);
        return 0;
    }

    /* Coloca o modelo na nossa grade (correspondência com as outras formas). */
    sh = &app.shapes[index];
    if (mesh.grid_file && mesh.vcount == VERTEX_COUNT) {
        /* Arquivo exportado por este programa: já está na grade. */
        ok = shape_from_grid(sh, mesh.positions, mesh.ncount == VERTEX_COUNT ? mesh.normals : NULL);
    } else {
        app_print("Encaixando o modelo na grade (%d triangulos)...", mesh.tcount);
        ok = shape_fit_mesh(sh, mesh.positions, mesh.vcount, mesh.tris, mesh.tcount);
    }
    obj_free(&mesh);
    if (ok != 1) {
        app_print(ok == 0 ? "Erro: memoria insuficiente." : "Erro: o modelo esta vazio.");
        return 0;
    }

    file_title(path, sh->name, sizeof(sh->name));
    sh->color = imported_color(index - SHAPE_BUILTIN_COUNT);
    app.shape_count++;

    /* A forma nova vira o destino (B): segurando -> a forma atual vira ela. */
    app.shape_b = index;
    app.t = 0.0f;
    app.choosing = CHOOSE_OFF;
    app_print("Forma importada: %s (tecla %d). Segure -> para transformar.", sh->name, index + 1);
    print_progress();
    return 1;
}

int app_export(const char *path) {
    char msg[300];
    int ok = export_model(path, &app.shapes[app.shape_a], &app.shapes[app.shape_b], app.t, msg,
                          sizeof(msg));
    app_print("%s", msg);
    return ok;
}

static void dialog_unavailable(void) {
    app_print("Erro: nenhuma janela de arquivos disponivel (instale o zenity).");
}

void app_import_dialog(void) {
    char path[1024];
    int r = dialog_open_file("Importar forma 3D", "Modelos 3D (OBJ)", "*.obj *.OBJ", path,
                             sizeof(path));
    if (r == DIALOG_OK) app_import(path);
    else if (r == DIALOG_UNAVAILABLE) dialog_unavailable();
}

void app_export_dialog(void) {
    char path[1024];
    int r = dialog_save_file("Exportar forma 3D", "modelo.obj", "Modelos 3D (OBJ)", "*.obj", path,
                             sizeof(path));
    if (r == DIALOG_OK) app_export(path);
    else if (r == DIALOG_UNAVAILABLE) dialog_unavailable();
}
