/* main.c - TF-CGR em OpenGL moderno (3.3 core).
 *
 * Projeto Final - Computação Gráfica - UDESC
 * Tema: Alteração de geometrias (morphing e deformação de objetos 3D)
 *
 * Funcionamento:
 *   1. Todas as formas são geradas na mesma grade paramétrica (shapes.c),
 *      então existe correspondência 1:1 entre os vértices.
 *   2. Cada forma fica em seu próprio VBO na GPU (enviado uma única vez).
 *   3. O VAO liga a forma A nos atributos 0/1 e a forma B nos
 *      atributos 2/3. Trocar o par = só religar ponteiros, sem reenviar dados.
 *   4. O vertex shader interpola posição e normal com o fator t.
 *   5. O painel (ui.c, Nuklear) e o teclado alteram o mesmo estado (app.c).
 *
 * Organização das pastas de src/:
 *   app/       estado do programa e lógica do morph
 *   geometry/  matemática, formas na grade, deformações, encaixe de modelos .obj
 *   render/    shaders e texturas (OpenGL)
 *   io/        arquivos: ler/escrever .obj, exportar, janela de escolher arquivo
 *   ui/        painel de controle (Nuklear)
 *
 * Este arquivo cuida da janela, do laço principal, da entrada e do desenho. */
#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app/app.h"
#include "geometry/mathutil.h"
#include "render/shader.h"
#include "geometry/shapes.h"
#include "render/texture.h"
#include "io/export.h"
#include "ui/ui.h"

#define WINDOW_W 1280
#define WINDOW_H 900     /* altura desejada (cabe o painel inteiro)... */
#define WINDOW_H_MIN 700 /* ...mas nunca maior que 90% do monitor */

/* ----------------------------------------------------------------------------
 * Configuração da geometria e dos shaders
 * ------------------------------------------------------------------------- */

/* Gera as formas, envia tudo para a GPU e configura o VAO. Retorna 0 se faltar memória. */
static int setup_geometry(void) {
    unsigned int *indices = (unsigned int *)malloc(INDEX_COUNT * sizeof(unsigned int));
    float *data = (float *)malloc(VERTEX_COUNT * 2 * sizeof(float)); /* UVs */
    int s, ok = 0;
    GLuint a;

    if (!indices || !data) goto done;
    app.shape_count = SHAPE_BUILTIN_COUNT; /* para o cleanup liberar o que foi alocado */
    if (!shapes_build(app.shapes)) goto done;
    grid_build_indices(indices);

    glGenVertexArrays(1, &app.vao);
    glBindVertexArray(app.vao);

    /* Índices: a topologia é a mesma para todas as formas. */
    glGenBuffers(1, &app.ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, INDEX_COUNT * sizeof(unsigned int), indices,
                 GL_STATIC_DRAW);

    /* Um VBO por forma, com [px py pz nx ny nz] por vértice. As formas
     * adicionadas depois (.obj) ganham o seu VBO do mesmo jeito. */
    for (s = 0; s < app.shape_count; ++s) app_upload_shape(s);

    /* Coordenadas de textura (u,v): um VBO só, ligado fixo no atributo 4,
     * porque são as mesmas para todas as formas. */
    grid_build_uvs(data);
    glGenBuffers(1, &app.uv_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, app.uv_vbo);
    glBufferData(GL_ARRAY_BUFFER, VERTEX_COUNT * 2 * sizeof(float), data, GL_STATIC_DRAW);
    glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);

    for (a = 0; a < 5; ++a) glEnableVertexAttribArray(a);
    glBindVertexArray(0);

    app_bind_shape_pair();
    ok = 1;
done:
    free(indices);
    free(data);
    return ok;
}

/* Consulta (uma única vez) a posição de cada uniform no programa. */
static void cache_uniforms(void) {
    GLuint p = app.program;
    app.loc.model = glGetUniformLocation(p, "uModel");
    app.loc.view = glGetUniformLocation(p, "uView");
    app.loc.proj = glGetUniformLocation(p, "uProj");
    app.loc.t = glGetUniformLocation(p, "uT");
    app.loc.color = glGetUniformLocation(p, "uColor");
    app.loc.light_pos = glGetUniformLocation(p, "uLightPos");
    app.loc.eye_pos = glGetUniformLocation(p, "uEyePos");
    app.loc.wireframe = glGetUniformLocation(p, "uWireframe");
    app.loc.wire_color = glGetUniformLocation(p, "uWireColor");
    app.loc.taper = glGetUniformLocation(p, "uTaper");
    app.loc.twist = glGetUniformLocation(p, "uTwist");
    app.loc.bend = glGetUniformLocation(p, "uBend");
    app.loc.tex = glGetUniformLocation(p, "uTex");
    app.loc.use_texture = glGetUniformLocation(p, "uUseTexture");
    app.loc.tex_multiply = glGetUniformLocation(p, "uTexMultiply");
    app.loc.tex_scale = glGetUniformLocation(p, "uTexScale");

    /* O sampler sempre lê da unidade de textura 0: basta definir uma vez. */
    glUseProgram(p);
    glUniform1i(app.loc.tex, 0);
    glUseProgram(0);
}

/* ----------------------------------------------------------------------------
 * Desenho
 * ------------------------------------------------------------------------- */

/* Desenha a cena na área (x0, 0, width, height) do framebuffer. */
static void render_scene(int x0, int width, int height) {
    const Camera *c = &app.cam;
    const MorphState *m = &app.morph;
    const TexState *tx = &app.tex;
    Vec3 eye, color;
    Mat4 view, proj, model;
    float t;
    int use_tex;

    if (width <= 0 || height <= 0) return;
    glViewport(x0, 0, width, height);
    glEnable(GL_DEPTH_TEST); /* o painel desliga o teste de profundidade a cada quadro */

    eye = v3(c->dist * cosf(c->pitch) * sinf(c->yaw), c->dist * sinf(c->pitch),
             c->dist * cosf(c->pitch) * cosf(c->yaw));
    view = m4_lookat(eye, v3(0, 0, 0), v3(0, 1, 0));
    proj = m4_perspective(0.8f, (float)width / height, 0.1f, 100.0f);
    model = m4_rotate_y(app.obj_angle);

    t = m->t;
    color = v3_lerp(app.shapes[m->from].color, app.shapes[m->to].color, t);

    glUseProgram(app.program);
    glUniformMatrix4fv(app.loc.model, 1, GL_FALSE, model.m);
    glUniformMatrix4fv(app.loc.view, 1, GL_FALSE, view.m);
    glUniformMatrix4fv(app.loc.proj, 1, GL_FALSE, proj.m);
    glUniform1f(app.loc.t, t);
    glUniform1f(app.loc.taper, app.deform.taper);
    glUniform1f(app.loc.twist, app.deform.twist);
    glUniform1f(app.loc.bend, app.deform.bend);

    /* Textura: "Nenhuma" usa só a cor da forma. */
    use_tex = tx->current != TEX_NONE && tx->ids[tx->current] != 0;
    glUniform1i(app.loc.use_texture, use_tex);
    glUniform1i(app.loc.tex_multiply, tx->multiply);
    glUniform1f(app.loc.tex_scale, tx->scale);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, use_tex ? tx->ids[tx->current] : 0);
    glUniform3f(app.loc.color, color.x, color.y, color.z);
    glUniform3f(app.loc.light_pos, 3.0f, 4.0f, 3.0f);
    glUniform3f(app.loc.eye_pos, eye.x, eye.y, eye.z);

    glBindVertexArray(app.vao);

    if (app.render_mode == RENDER_SOLID || app.render_mode == RENDER_SOLID_WIRE) {
        /* Empurra o sólido um pouco para trás para a malha aparecer por cima. */
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1.0f, 1.0f);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glUniform1i(app.loc.wireframe, 0);
        glDrawElements(GL_TRIANGLES, INDEX_COUNT, GL_UNSIGNED_INT, 0);
        glDisable(GL_POLYGON_OFFSET_FILL);
    }
    if (app.render_mode == RENDER_SOLID_WIRE || app.render_mode == RENDER_WIRE) {
        /* Linhas escuras sobre o sólido; claras quando só a malha aparece. */
        Vec3 wc = (app.render_mode == RENDER_WIRE) ? color : v3_scale(color, 0.35f);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glUniform1i(app.loc.wireframe, 1);
        glUniform3f(app.loc.wire_color, wc.x, wc.y, wc.z);
        glDrawElements(GL_TRIANGLES, INDEX_COUNT, GL_UNSIGNED_INT, 0);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    glBindVertexArray(0);
}

static void update_title(GLFWwindow *win, float fps) {
    static const char *mode_names[] = {"sólido", "sólido+malha", "malha"};
    const MorphState *m = &app.morph;
    char title[320], next[64] = "";
    if (m->queued >= 0) snprintf(next, sizeof(next), " (próximo: %s)", app.shapes[m->queued].name);
    snprintf(title, sizeof(title),
             "TF-CGR | %s -> %s%s | t = %.2f | %s | %s | %.0f FPS",
             app.shapes[m->from].name, app.shapes[m->to].name, next, m->t,
             m->auto_play ? "automático" : "pausado",
             mode_names[app.render_mode], fps);
    glfwSetWindowTitle(win, title);
}

static void print_help(void) {
    int i;
    printf("\n=== TF-CGR - Controles ===\n"
           "  Painel a esquerda: formas, mistura, deformacoes, texturas e exportacao\n"
           "  ESPACO        liga / pausa a animacao automatica\n"
           "  SETAS < >     controla o morph manualmente (t diminui / aumenta)\n"
           "  1 a %d         escolhe a proxima forma de destino (5 = Plano)\n"
           "  W             alterna solido / solido+malha / so malha\n"
           "  R             liga / desliga a rotacao do objeto\n"
           "  Mouse (esq.)  arrasta para orbitar a camera\n"
           "  Scroll        zoom\n"
           "  ESC           sair\n"
           "  Arraste um .obj para a janela para adiciona-lo como forma\n\n"
           "Formas:",
           app.shape_count);
    for (i = 0; i < app.shape_count; ++i) printf("  [%d] %s", i + 1, app.shapes[i].name);
    printf("\n\n");
}

/* Salva o framebuffer atual num arquivo PPM (usado pela opção --captura). */
static int save_ppm(const char *path, int w, int h) {
    unsigned char *px = (unsigned char *)malloc((size_t)w * h * 3);
    FILE *f;
    int y;
    if (!px) return 0;
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, px);
    f = fopen(path, "wb");
    if (!f) {
        free(px);
        return 0;
    }
    fprintf(f, "P6\n%d %d\n255\n", w, h);
    for (y = h - 1; y >= 0; --y) fwrite(px + (size_t)y * w * 3, 1, (size_t)w * 3, f); /* de cima para baixo */
    fclose(f);
    free(px);
    return 1;
}

/* ----------------------------------------------------------------------------
 * Callbacks de entrada
 * Todos repassam o evento ao painel primeiro; a cena só reage se o
 * painel não estiver usando o mouse/teclado.
 * ------------------------------------------------------------------------- */
static void key_callback(GLFWwindow *win, int key, int scancode, int action, int mods) {
    ui_on_key(win, key, scancode, action, mods);
    if (action != GLFW_PRESS) return;

    switch (key) {
        case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(win, GLFW_TRUE); break;
        case GLFW_KEY_SPACE:  app.morph.auto_play = !app.morph.auto_play; break;
        case GLFW_KEY_W:      app.render_mode = (app.render_mode + 1) % RENDER_MODE_COUNT; break;
        case GLFW_KEY_R:      app.rotate = !app.rotate; break;
        default:
            /* Números de cima do teclado ou do teclado numérico: 1 = Esfera ... 5 = Plano. */
            if (key >= GLFW_KEY_1 && key <= GLFW_KEY_9 && key - GLFW_KEY_1 < app.shape_count)
                app_choose_target(key - GLFW_KEY_1);
            else if (key >= GLFW_KEY_KP_1 && key <= GLFW_KEY_KP_9 && key - GLFW_KEY_KP_1 < app.shape_count)
                app_choose_target(key - GLFW_KEY_KP_1);
    }
}

static void char_callback(GLFWwindow *win, unsigned int codepoint) { ui_on_char(win, codepoint); }

static void mouse_button_callback(GLFWwindow *win, int button, int action, int mods) {
    ui_on_mouse_button(win, button, action, mods);
    if (button != GLFW_MOUSE_BUTTON_LEFT) return;
    if (action == GLFW_PRESS) {
        /* Clique no painel não começa a orbitar a câmera. */
        app.cam.dragging = !ui_wants_mouse();
        glfwGetCursorPos(win, &app.cam.last_x, &app.cam.last_y);
    } else {
        app.cam.dragging = 0;
    }
}

static void cursor_callback(GLFWwindow *win, double x, double y) {
    Camera *c = &app.cam;
    (void)win;
    if (!c->dragging) return;
    c->yaw -= (float)(x - c->last_x) * 0.01f;
    c->pitch += (float)(y - c->last_y) * 0.01f;
    c->pitch = clampf(c->pitch, -1.5f, 1.5f);
    c->last_x = x;
    c->last_y = y;
}

/* Diz se o caminho termina com a extensão dada (sem diferenciar maiúsculas). */
static int has_extension(const char *path, const char *ext) {
    size_t n = strlen(path), e = strlen(ext), i;
    if (n < e) return 0;
    for (i = 0; i < e; ++i) {
        char c = path[n - e + i];
        if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
        if (c != ext[i]) return 0;
    }
    return 1;
}

/* Arrastar um .obj para a janela adiciona o modelo como forma. */
static void drop_callback(GLFWwindow *win, int count, const char **paths) {
    (void)win;
    if (count <= 0) return;
    if (has_extension(paths[0], ".obj")) app_add_shape_file(paths[0]);
    else app_message(&app.shape_msg, 1, "Arraste um arquivo .obj para adicionar uma forma.");
}

static void scroll_callback(GLFWwindow *win, double dx, double dy) {
    ui_on_scroll(win, dx, dy);
    if (ui_wants_mouse()) return; /* scroll sobre o painel rola o painel */
    app.cam.dist = clampf(app.cam.dist - (float)dy * 0.3f, CAM_DIST_MIN, CAM_DIST_MAX);
}

/* ----------------------------------------------------------------------------
 * Limpeza
 * ------------------------------------------------------------------------- */
static void cleanup(void) {
    int i;
    ui_shutdown();
    for (i = 0; i < TEX_COUNT; ++i)
        if (app.tex.ids[i]) glDeleteTextures(1, &app.tex.ids[i]);
    if (app.uv_vbo) glDeleteBuffers(1, &app.uv_vbo);
    for (i = 0; i < app.shape_count; ++i) {
        if (app.vbos[i]) glDeleteBuffers(1, &app.vbos[i]);
        shape_free(&app.shapes[i]);
    }
    if (app.ebo) glDeleteBuffers(1, &app.ebo);
    if (app.vao) glDeleteVertexArrays(1, &app.vao);
    if (app.program) glDeleteProgram(app.program);
}

/* --------------------------------------------------------------------------*/
int main(int argc, char **argv) {
    GLFWwindow *win;
    const GLFWvidmode *mode;
    int win_height = WINDOW_H;
    char vert_path[512], frag_path[512];
    /* Opções de linha de comando (todas opcionais):
     *   --forma modelo.obj    adiciona o modelo como forma
     *   --exportar saida.obj  exporta a forma exibida logo ao abrir
     *   --captura arq.ppm     salva um quadro da tela e sai (testes)
     *   --quadros N           qual quadro capturar (passo fixo de 1/60 s) */
    const char *capture_path = NULL, *shape_path = NULL, *export_path = NULL;
    int capture_frame = 100;
    double last, title_timer = 0.0;
    int frames = 0, frame_number = 0, i;

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--captura") == 0 && i + 1 < argc) capture_path = argv[++i];
        else if (strcmp(argv[i], "--quadros") == 0 && i + 1 < argc) capture_frame = atoi(argv[++i]);
        else if (strcmp(argv[i], "--forma") == 0 && i + 1 < argc) shape_path = argv[++i];
        else if (strcmp(argv[i], "--exportar") == 0 && i + 1 < argc) export_path = argv[++i];
        else fprintf(stderr, "Aviso: opcao desconhecida '%s'\n", argv[i]);
    }

    memset(&app, 0, sizeof(app));
    app_reset_state();

    if (!glfwInit()) {
        fprintf(stderr, "Erro: falha ao iniciar o GLFW\n");
        return 1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE); /* necessário no macOS */
    glfwWindowHint(GLFW_SAMPLES, 4);                       /* antialiasing */

    mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    if (mode && mode->height * 9 / 10 < win_height) win_height = mode->height * 9 / 10;
    if (win_height < WINDOW_H_MIN) win_height = WINDOW_H_MIN;

    win = glfwCreateWindow(WINDOW_W, win_height, "TF-CGR", NULL, NULL);
    if (!win) {
        fprintf(stderr, "Erro: nao foi possivel criar janela OpenGL 3.3\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(win);
    glfwSwapInterval(1); /* vsync */

    if (!gladLoadGL(glfwGetProcAddress)) {
        fprintf(stderr, "Erro: falha ao carregar funcoes OpenGL (GLAD)\n");
        glfwTerminate();
        return 1;
    }
    printf("OpenGL %s | %s\n", (const char *)glGetString(GL_VERSION),
           (const char *)glGetString(GL_RENDERER));

    glfwSetKeyCallback(win, key_callback);
    glfwSetCharCallback(win, char_callback);
    glfwSetMouseButtonCallback(win, mouse_button_callback);
    glfwSetCursorPosCallback(win, cursor_callback);
    glfwSetScrollCallback(win, scroll_callback);
    glfwSetDropCallback(win, drop_callback);

    glEnable(GL_MULTISAMPLE);

    shader_find("morph.vert", vert_path, sizeof(vert_path));
    shader_find("morph.frag", frag_path, sizeof(frag_path));
    app.program = shader_load_program(vert_path, frag_path);
    if (!app.program || !setup_geometry() || !texture_make_presets(app.tex.ids) || !ui_init(win)) {
        fprintf(stderr, "Erro: falha na inicializacao (shaders, memoria ou painel)\n");
        cleanup();
        glfwDestroyWindow(win);
        glfwTerminate();
        return 1;
    }
    cache_uniforms();
    print_help();
    if (shape_path) app_add_shape_file(shape_path);
    if (export_path) export_model(export_path);

    last = glfwGetTime();
    while (!glfwWindowShouldClose(win)) {
        int fb_w, fb_h, win_w, win_h, panel_px, manual_dir = 0;
        double now = glfwGetTime();
        float dt = (float)(now - last);
        last = now;
        /* Depois de uma janela de arquivo (que congela o programa) o dt
         * seria enorme e o morph pularia: limitamos o passo. */
        if (dt > 0.1f) dt = 0.1f;
        /* Na captura usamos passo fixo, para o resultado ser reprodutível. */
        if (capture_path) dt = 1.0f / 60.0f;

        glfwPollEvents();
        ui_frame(); /* painel primeiro: ele pode alterar o estado deste quadro */

        /* Setas: controle manual do morph. */
        if (glfwGetKey(win, GLFW_KEY_RIGHT) == GLFW_PRESS) manual_dir += 1;
        if (glfwGetKey(win, GLFW_KEY_LEFT) == GLFW_PRESS) manual_dir -= 1;
        app_update(dt, manual_dir);

        /* A cena ocupa a área à direita do painel (em pixels do framebuffer). */
        glfwGetFramebufferSize(win, &fb_w, &fb_h);
        glfwGetWindowSize(win, &win_w, &win_h);
        panel_px = win_w > 0 ? (int)(ui_panel_width() * fb_w / win_w) : 0;

        glViewport(0, 0, fb_w, fb_h);
        glClearColor(0.08f, 0.09f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        render_scene(panel_px, fb_w - panel_px, fb_h);
        ui_render();

        if (capture_path && ++frame_number == capture_frame) {
            if (save_ppm(capture_path, fb_w, fb_h)) printf("Captura salva em %s\n", capture_path);
            else fprintf(stderr, "Erro: nao foi possivel salvar %s\n", capture_path);
            glfwSetWindowShouldClose(win, GLFW_TRUE);
        }
        glfwSwapBuffers(win);

        /* Atualiza o título (estado + FPS) 4 vezes por segundo. */
        ++frames;
        title_timer += dt;
        if (title_timer >= 0.25) {
            update_title(win, frames / (float)title_timer);
            frames = 0;
            title_timer = 0.0;
        }
    }

    cleanup();
    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}
