/* main.c - TF-CGR: morph de objetos 3D em C99 + OpenGL 3.3.
 *
 * Projeto Final - Computação Gráfica - UDESC
 * Tema: Alteração de geometrias (morphing de objetos 3D)
 *
 * Como o morph funciona (detalhes em geometry/morph.h):
 *   1. Todas as formas são geradas na MESMA grade (u, v) (shapes.c), então
 *      o vértice i de uma forma corresponde ao vértice i de qualquer outra.
 *      Modelos .obj são projetados nessa grade (mesh_fit.c).
 *   2. A cada quadro, morph.c calcula em C a forma intermediária
 *      P(t) = (1 - t) * A + t * B, vértice a vértice.
 *   3. render.c envia o resultado para a GPU, que só desenha.
 *
 * Organização de src/:
 *   main.c     janela, teclado/mouse e laço principal (este arquivo)
 *   app/       estado do programa e o que cada tecla faz
 *   geometry/  matemática, formas na grade, encaixe de .obj e o MORPH
 *   render/    shaders e desenho (OpenGL)
 *   io/        ler/escrever .obj, exportar, janela de escolher arquivo
 *
 * Bibliotecas externas: só GLFW (cria a janela e lê o teclado) e glad
 * (carrega as funções do OpenGL). Todo o resto é código nosso. */
#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app/app.h"
#include "render/render.h"

#define WINDOW_W 1280
#define WINDOW_H 720

/* ============================================================================
 * Entrada: teclado e mouse
 * ========================================================================== */

/* Teclas de toque único (uma ação por aperto). As teclas que funcionam
 * "segurando" (setas, vírgula e ponto) são lidas no laço principal. */
static void key_callback(GLFWwindow *win, int key, int scancode, int action, int mods) {
    (void)scancode;
    (void)mods;
    if (action != GLFW_PRESS) return;

    switch (key) {
        case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(win, GLFW_TRUE); break;
        case GLFW_KEY_Q:      app_start_choosing(); break;
        case GLFW_KEY_W:      app_toggle_wireframe(); break;
        case GLFW_KEY_R:      app_toggle_rotation(); break;
        case GLFW_KEY_I:      app_import_dialog(); break;
        case GLFW_KEY_O:      app_export_dialog(); break;
        case GLFW_KEY_H:      app_print_help(); break;
        default:
            /* Números de 1 a 9 (em cima das letras ou no teclado numérico). */
            if (key >= GLFW_KEY_1 && key <= GLFW_KEY_9) app_number_key(key - GLFW_KEY_1);
            else if (key >= GLFW_KEY_KP_1 && key <= GLFW_KEY_KP_9) app_number_key(key - GLFW_KEY_KP_1);
    }
}

/* Mouse: segurar o botão esquerdo e arrastar gira a câmera. */
static void mouse_button_callback(GLFWwindow *win, int button, int action, int mods) {
    (void)mods;
    if (button != GLFW_MOUSE_BUTTON_LEFT) return;
    app.cam.dragging = (action == GLFW_PRESS);
    glfwGetCursorPos(win, &app.cam.last_x, &app.cam.last_y);
}

static void cursor_callback(GLFWwindow *win, double x, double y) {
    Camera *c = &app.cam;
    (void)win;
    if (!c->dragging) return;
    c->yaw -= (float)(x - c->last_x) * 0.01f;   /* horizontal: gira em volta */
    c->pitch += (float)(y - c->last_y) * 0.01f; /* vertical: sobe/desce */
    c->pitch = clampf(c->pitch, -1.5f, 1.5f);   /* não passa por cima do polo */
    c->last_x = x;
    c->last_y = y;
}

/* Arrastar um arquivo .obj para a janela também importa a forma. */
static void drop_callback(GLFWwindow *win, int count, const char **paths) {
    (void)win;
    if (count > 0) app_import(paths[0]);
}

/* Título da janela: par de formas e porcentagem do morph. */
static void update_title(GLFWwindow *win) {
    static char last[160];
    char title[160];
    snprintf(title, sizeof(title), "TF-CGR | %s -> %s | %.0f%%", app.shapes[app.shape_a].name,
             app.shapes[app.shape_b].name, app.t * 100.0f);
    if (strcmp(title, last) != 0) { /* só troca quando mudou */
        glfwSetWindowTitle(win, title);
        snprintf(last, sizeof(last), "%s", title);
    }
}

/* Salva a imagem da janela num arquivo PPM (opção --captura, para testes). */
static int save_ppm(const char *path, int w, int h) {
    unsigned char *px = (unsigned char *)malloc((size_t)w * h * 3);
    FILE *f;
    int y;
    if (!px) return 0;
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, px);
    f = fopen(path, "wb");
    if (f) {
        fprintf(f, "P6\n%d %d\n255\n", w, h);
        for (y = h - 1; y >= 0; --y) fwrite(px + (size_t)y * w * 3, 1, (size_t)w * 3, f);
        fclose(f);
    }
    free(px);
    return f != NULL;
}

/* ============================================================================
 * Programa principal
 * ========================================================================== */
int main(int argc, char **argv) {
    GLFWwindow *win;
    const GLFWvidmode *mode;
    int win_h = WINDOW_H, frame = 0, capture_frame = 100, i;
    double last;
    /* Opções de linha de comando (todas opcionais):
     *   --forma modelo.obj    importa o modelo como forma
     *   --mistura T           começa com o morph em T (0 a 1)
     *   --exportar saida.obj  exporta a forma exibida logo ao abrir
     *   --captura arq.ppm     salva um quadro da tela e sai (testes)
     *   --quadros N           qual quadro capturar */
    const char *shape_path = NULL, *export_path = NULL, *capture_path = NULL;
    float start_t = 0.0f;

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--forma") == 0 && i + 1 < argc) shape_path = argv[++i];
        else if (strcmp(argv[i], "--mistura") == 0 && i + 1 < argc) start_t = (float)atof(argv[++i]);
        else if (strcmp(argv[i], "--exportar") == 0 && i + 1 < argc) export_path = argv[++i];
        else if (strcmp(argv[i], "--captura") == 0 && i + 1 < argc) capture_path = argv[++i];
        else if (strcmp(argv[i], "--quadros") == 0 && i + 1 < argc) capture_frame = atoi(argv[++i]);
        else fprintf(stderr, "Aviso: opcao desconhecida '%s'\n", argv[i]);
    }

    /* ---- Janela OpenGL 3.3 (GLFW) e funções do OpenGL (glad) ------------- */
    if (!glfwInit()) {
        fprintf(stderr, "Erro: falha ao iniciar o GLFW\n");
        return 1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE); /* necessário no macOS */
    glfwWindowHint(GLFW_SAMPLES, 4);                       /* antialiasing */

    /* Em telas pequenas (ex.: 1366x768) a janela usa no máximo 90% da altura. */
    mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    if (mode && mode->height * 9 / 10 < win_h) win_h = mode->height * 9 / 10;

    win = glfwCreateWindow(WINDOW_W, win_h, "TF-CGR", NULL, NULL);
    if (!win) {
        fprintf(stderr, "Erro: nao foi possivel criar janela OpenGL 3.3\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(win);
    glfwSwapInterval(1); /* vsync */
    if (!gladLoadGL(glfwGetProcAddress)) {
        fprintf(stderr, "Erro: falha ao carregar funcoes OpenGL (glad)\n");
        glfwTerminate();
        return 1;
    }

    glfwSetKeyCallback(win, key_callback);
    glfwSetMouseButtonCallback(win, mouse_button_callback);
    glfwSetCursorPosCallback(win, cursor_callback);
    glfwSetDropCallback(win, drop_callback);

    /* ---- Estado, formas e desenho --------------------------------------- */
    if (!app_init() || !render_init()) {
        fprintf(stderr, "Erro: falha na inicializacao (shaders ou memoria)\n");
        render_shutdown();
        app_shutdown();
        glfwTerminate();
        return 1;
    }
    if (shape_path) app_import(shape_path);
    app.t = clampf(start_t, 0.0f, 1.0f);
    if (export_path) app_export(export_path);
    app_print_help();

    /* ---- Laço principal -------------------------------------------------- */
    last = glfwGetTime();
    while (!glfwWindowShouldClose(win)) {
        int width, height, morph_dir = 0, zoom_dir = 0;
        double now = glfwGetTime();
        float dt = (float)(now - last); /* segundos desde o quadro anterior */
        last = now;
        /* Depois de uma janela de arquivo (que congela o programa) o dt
         * seria enorme: limitamos o passo. */
        if (dt > 0.1f) dt = 0.1f;
        if (capture_path) dt = 1.0f / 60.0f; /* captura: passo fixo */

        glfwPollEvents(); /* chama os callbacks acima */

        /* Teclas seguradas: setas = morph, vírgula/ponto = zoom. */
        if (glfwGetKey(win, GLFW_KEY_RIGHT) == GLFW_PRESS) morph_dir += 1;
        if (glfwGetKey(win, GLFW_KEY_LEFT) == GLFW_PRESS) morph_dir -= 1;
        if (glfwGetKey(win, GLFW_KEY_COMMA) == GLFW_PRESS) zoom_dir -= 1;  /* aproxima */
        if (glfwGetKey(win, GLFW_KEY_PERIOD) == GLFW_PRESS) zoom_dir += 1; /* afasta */
        app_update(dt, morph_dir, zoom_dir);

        glfwGetFramebufferSize(win, &width, &height);
        render_frame(width, height);
        update_title(win);

        if (capture_path && ++frame == capture_frame) {
            if (save_ppm(capture_path, width, height)) app_print("Captura salva em %s", capture_path);
            glfwSetWindowShouldClose(win, GLFW_TRUE);
        }
        glfwSwapBuffers(win);
    }

    app_print("Saindo.");
    render_shutdown();
    app_shutdown();
    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}
