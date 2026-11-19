/* ui.c - Painel de controle (Nuklear) do Laboratório de Deformação.
 *
 * O Nuklear é uma interface "modo imediato": a cada quadro o painel
 * inteiro é descrito de novo, lendo e escrevendo direto no estado 'app'.
 * Não existe "objeto botão" guardado; um botão é só um if. */
#include "ui/ui.h"

#include <stdio.h>

#include "app/app.h"
#include "io/export.h"
#include "io/file_dialog.h"
#include "ui/nuklear_setup.h"

#define PANEL_NAME "painel"
#define PANEL_TITLE "Laboratório de Deformação"
#define PANEL_WIDTH 330.0f
#define ROW_H 20.0f

/* Tamanho máximo dos buffers de vértices/índices do painel na GPU. */
#define MAX_VERTEX_BUFFER (512 * 1024)
#define MAX_ELEMENT_BUFFER (128 * 1024)

static struct nk_glfw nk_glfw_state; /* estado do backend GLFW + OpenGL 3 */
static struct nk_context *ctx;
static int panel_open = 1; /* painel expandido (não minimizado)? */

/* ----------------------------------------------------------------------------
 * Inicialização / eventos
 * ------------------------------------------------------------------------- */
/* Ajustes de estilo: no tema padrão do Nuklear uma checkbox marcada fica
 * escura (parece desmarcada). Usamos uma cor de destaque para "ligado". */
static void setup_style(void) {
    const struct nk_color accent = nk_rgb(90, 160, 240), accent_hover = nk_rgb(130, 185, 250);
    struct nk_style *st = &ctx->style;
    st->checkbox.cursor_normal = nk_style_item_color(accent);
    st->checkbox.cursor_hover = nk_style_item_color(accent_hover);
    st->option.cursor_normal = nk_style_item_color(accent);
    st->option.cursor_hover = nk_style_item_color(accent_hover);
    st->slider.cursor_normal = nk_style_item_color(accent);
    st->slider.cursor_hover = nk_style_item_color(accent_hover);
    st->slider.cursor_active = nk_style_item_color(accent_hover);
    st->window.spacing = nk_vec2(4, 3); /* painel mais compacto (telas de 768 px) */
}

int ui_init(GLFWwindow *win) {
    struct nk_font_atlas *atlas;
    struct nk_font *font;

    /* NK_GLFW3_DEFAULT: o backend NÃO instala callbacks próprios; os nossos
     * callbacks (main.c) repassam os eventos com as funções ui_on_*. */
    ctx = nk_glfw3_init(&nk_glfw_state, win, NK_GLFW3_DEFAULT);
    if (!ctx) return 0;
    /* Correção do backend: ele guarda o endereço de uma variável local como
     * dado da área de transferência (Ctrl+C/Ctrl+V travariam o programa). */
    ctx->clip.userdata = nk_handle_ptr(&nk_glfw_state);

    nk_glfw3_font_stash_begin(&nk_glfw_state, &atlas);
    font = nk_font_atlas_add_default(atlas, 15.0f, NULL); /* ProggyClean, Latin-1 */
    nk_glfw3_font_stash_end(&nk_glfw_state);
    if (font) nk_style_set_font(ctx, &font->handle);
    setup_style();
    return 1;
}

void ui_shutdown(void) {
    if (ctx) nk_glfw3_shutdown(&nk_glfw_state);
    ctx = NULL;
}

void ui_on_key(GLFWwindow *win, int key, int scancode, int action, int mods) {
    nk_glfw3_key_callback(win, key, scancode, action, mods);
}
void ui_on_char(GLFWwindow *win, unsigned int codepoint) { nk_glfw3_char_callback(win, codepoint); }
void ui_on_scroll(GLFWwindow *win, double dx, double dy) { nk_gflw3_scroll_callback(win, dx, dy); }
void ui_on_mouse_button(GLFWwindow *win, int button, int action, int mods) {
    nk_glfw3_mouse_button_callback(win, button, action, mods);
}

int ui_wants_mouse(void) { return ctx && nk_window_is_any_hovered(ctx); }
float ui_panel_width(void) { return panel_open ? PANEL_WIDTH : 0.0f; }

/* ----------------------------------------------------------------------------
 * Pequenos componentes reutilizados
 * ------------------------------------------------------------------------- */

/* Linha "rótulo valor | slider". Retorna 1 se o usuário mudou o valor. */
static int slider_row(const char *label, float *value, float lo, float hi, float step) {
    static const float ratio[] = {0.42f, 0.58f};
    char text[64];
    nk_layout_row(ctx, NK_DYNAMIC, ROW_H, 2, ratio);
    snprintf(text, sizeof(text), "%s %.2f", label, *value);
    nk_label(ctx, text, NK_TEXT_LEFT);
    return nk_slider_float(ctx, lo, value, hi, step);
}

/* Linha "rótulo | combo de formas". Retorna a forma escolhida. */
static int shape_combo_row(const char *label, int selected) {
    static const float ratio[] = {0.42f, 0.58f};
    const char *names[MAX_SHAPES];
    int i;
    for (i = 0; i < app.shape_count; ++i) names[i] = app.shapes[i].name;
    nk_layout_row(ctx, NK_DYNAMIC, ROW_H, 2, ratio);
    nk_label(ctx, label, NK_TEXT_LEFT);
    return nk_combo(ctx, names, app.shape_count, selected, (int)ROW_H,
                    nk_vec2(180, (ROW_H + 4) * app.shape_count + 8));
}

/* Mensagem de resultado (verde = ok, vermelho = erro), com quebra de linha. */
static void message_row(const Message *m) {
    if (!m->text[0]) return;
    nk_layout_row_dynamic(ctx, 34, 1);
    nk_label_colored_wrap(ctx, m->text, m->is_error ? nk_rgb(255, 110, 100) : nk_rgb(130, 220, 130));
}

/* Botão de largura total. */
static int button_row(const char *label) {
    nk_layout_row_dynamic(ctx, ROW_H + 2, 1);
    return nk_button_label(ctx, label);
}

/* Avisa no painel quando não há janela de arquivos no sistema. */
static void no_dialog(Message *m) {
    app_message(m, 1, "Erro: nenhuma janela de arquivos disponível (instale o zenity).");
}

/* Checkbox ligada direto a um int do estado. Retorna 1 se mudou. */
static int checkbox_row(const char *label, int *value) {
    nk_bool v = *value ? nk_true : nk_false;
    int changed;
    nk_layout_row_dynamic(ctx, ROW_H, 1);
    changed = nk_checkbox_label(ctx, label, &v);
    *value = v ? 1 : 0;
    return changed;
}

/* ----------------------------------------------------------------------------
 * Seções do painel
 * ------------------------------------------------------------------------- */
static void section_shapes(void) {
    MorphState *m = &app.morph;
    int a, b;
    float t = m->t;
    char text[96];

    a = shape_combo_row("Forma A", m->from);
    b = shape_combo_row("Forma B", m->to);
    if (a != m->from || b != m->to) app_set_shapes(a, b);

    if (slider_row("t", &t, 0.0f, 1.0f, 0.001f)) app_set_t(t);

    nk_layout_row_dynamic(ctx, ROW_H, 1);
    snprintf(text, sizeof(text), "Mistura exibida: %.0f%% de %s", m->t * 100.0f,
             app.shapes[m->to].name);
    nk_label(ctx, text, NK_TEXT_LEFT);
    if (m->queued >= 0) {
        snprintf(text, sizeof(text), "Próximo destino: %s", app.shapes[m->queued].name);
        nk_label_colored(ctx, text, NK_TEXT_LEFT, nk_rgb(240, 200, 90));
    }

    checkbox_row("Animação automática (ESPAÇO)", &m->auto_play);

    /* Abre a janela do sistema para escolher um modelo 3D (.obj). */
    if (button_row("Adicionar forma (.obj)...")) {
        char path[1024];
        int r = dialog_open_file("Adicionar forma 3D", "Modelos 3D (OBJ)", "*.obj *.OBJ", path,
                                 sizeof(path));
        if (r == DIALOG_OK) app_add_shape_file(path);
        else if (r == DIALOG_UNAVAILABLE) no_dialog(&app.shape_msg);
    }
    message_row(&app.shape_msg);
}

static void section_deform(void) {
    Deform *d = &app.deform;
    slider_row("Taper", &d->taper, TAPER_MIN, TAPER_MAX, 0.01f);
    slider_row("Twist", &d->twist, TWIST_MIN, TWIST_MAX, 0.01f);
    slider_row("Bend", &d->bend, BEND_MIN, BEND_MAX, 0.01f);
    nk_layout_row_dynamic(ctx, 16, 1);
    nk_label_colored(ctx, "Ordem: taper, twist, bend (eixo Y)", NK_TEXT_LEFT,
                     nk_rgb(150, 150, 150));
    nk_layout_row_dynamic(ctx, ROW_H + 2, 1);
    if (nk_button_label(ctx, "Zerar deformações")) app_reset_deform();
}

static void section_texture(void) {
    static const float ratio[] = {0.42f, 0.58f};
    TexState *tx = &app.tex;
    const char *names[TEX_COUNT];
    int i, sel;

    for (i = 0; i < TEX_COUNT; ++i) names[i] = texture_name(i);
    nk_layout_row(ctx, NK_DYNAMIC, ROW_H, 2, ratio);
    nk_label(ctx, "Textura", NK_TEXT_LEFT);
    sel = nk_combo(ctx, names, TEX_COUNT, tx->current, (int)ROW_H, nk_vec2(180, 160));
    if (sel != tx->current) {
        if (sel == TEX_IMAGE && !tx->ids[TEX_IMAGE]) {
            app_message(&tx->msg, 1, "Nenhuma imagem carregada ainda: use o botão abaixo.");
        } else {
            tx->current = sel;
        }
    }

    slider_row("Repetição", &tx->scale, TEX_SCALE_MIN, TEX_SCALE_MAX, 0.05f);
    checkbox_row("Multiplicar pela cor da forma", &tx->multiply);

    /* Abre a janela do sistema para escolher a imagem. Arrastar o arquivo
     * para a janela também funciona (drop_callback em main.c). */
    if (button_row("Adicionar imagem (PNG/JPG)...")) {
        char path[1024];
        int r = dialog_open_file("Escolher imagem de textura", "Imagens (PNG, JPG)",
                                 "*.png *.jpg *.jpeg *.PNG *.JPG *.JPEG", path, sizeof(path));
        if (r == DIALOG_OK) app_load_image(path);
        else if (r == DIALOG_UNAVAILABLE) no_dialog(&tx->msg);
    }

    if (tx->ids[TEX_IMAGE]) {
        char text[128];
        nk_layout_row_dynamic(ctx, 16, 1);
        snprintf(text, sizeof(text), "Imagem: %s", tx->image_info);
        nk_label_colored(ctx, text, NK_TEXT_LEFT, nk_rgb(150, 150, 150));
    }
    message_row(&tx->msg);
}

static void section_view(void) {
    static const char *modes[RENDER_MODE_COUNT] = {"Sólido", "Sólido + malha", "Só malha"};
    static const float ratio[] = {0.42f, 0.58f};
    nk_bool rotate = app.rotate ? nk_true : nk_false;

    nk_layout_row(ctx, NK_DYNAMIC, ROW_H, 2, ratio);
    nk_label(ctx, "Desenho (W)", NK_TEXT_LEFT);
    app.render_mode = nk_combo(ctx, modes, RENDER_MODE_COUNT, app.render_mode, (int)ROW_H,
                               nk_vec2(180, 100));

    slider_row("Distância", &app.cam.dist, CAM_DIST_MIN, CAM_DIST_MAX, 0.05f);

    nk_layout_row_dynamic(ctx, ROW_H + 2, 2);
    nk_checkbox_label(ctx, "Rotação (R)", &rotate);
    app.rotate = rotate ? 1 : 0;
    if (nk_button_label(ctx, "Resetar câmera")) app_reset_camera();
}

static void section_help(void) {
    static const char *lines[] = {
        "ESPAÇO   liga / pausa a animação",
        "<- ->    controla o morph (segurar)",
        NULL, /* teclas das formas: montada abaixo */
        "W        modo de desenho (malha)",
        "R        rotação do objeto",
        "Mouse    arrastar gira, scroll zoom",
        "ESC      sair",
    };
    char keys[64];
    size_t i;
    /* "1 a 5" com as formas prontas; cresce quando uma forma é adicionada. */
    snprintf(keys, sizeof(keys), "1 a %d    escolhe a próxima forma", app.shape_count);
    nk_layout_row_dynamic(ctx, 16, 1);
    for (i = 0; i < sizeof(lines) / sizeof(lines[0]); ++i)
        nk_label(ctx, lines[i] ? lines[i] : keys, NK_TEXT_LEFT);
}

/* Fim do painel: resetar tudo e exportar o modelo 3D. */
static void section_bottom(void) {
    nk_layout_row_dynamic(ctx, 6, 1);
    nk_spacing(ctx, 1);
    nk_layout_row_dynamic(ctx, ROW_H + 6, 2);
    if (nk_widget_is_hovered(ctx)) nk_tooltip(ctx, "Volta tudo ao estado inicial (as formas adicionadas continuam)");
    if (nk_button_label(ctx, "Resetar tudo")) app_reset_state();
    if (nk_widget_is_hovered(ctx)) nk_tooltip(ctx, "Salva a forma que está na tela (com as deformações) num arquivo .obj");
    if (nk_button_label(ctx, "Exportar 3D (.obj)...")) {
        char path[1024];
        int r = dialog_save_file("Exportar modelo 3D", "modelo.obj", "Modelos 3D (OBJ)", "*.obj",
                                 path, sizeof(path));
        if (r == DIALOG_OK) export_model(path);
        else if (r == DIALOG_UNAVAILABLE) no_dialog(&app.export_msg);
    }
    message_row(&app.export_msg);
}

/* ----------------------------------------------------------------------------
 * Quadro
 * ------------------------------------------------------------------------- */
void ui_frame(void) {
    int win_w, win_h;
    struct nk_rect bounds;
    const nk_flags flags = NK_WINDOW_BORDER | NK_WINDOW_TITLE | NK_WINDOW_MINIMIZABLE;

    nk_glfw3_new_frame(&nk_glfw_state);

    /* Painel preso à esquerda, com a altura da janela. */
    glfwGetWindowSize(nk_glfw_state.win, &win_w, &win_h);
    bounds = nk_rect(0, 0, PANEL_WIDTH, (float)win_h);
    if (nk_window_find(ctx, PANEL_NAME)) nk_window_set_bounds(ctx, PANEL_NAME, bounds);

    /* Ao abrir, só "Forma e mistura" e "Atalhos" vêm expandidas. */
    if (nk_begin_titled(ctx, PANEL_NAME, PANEL_TITLE, bounds, flags)) {
        if (nk_tree_push(ctx, NK_TREE_TAB, "Forma e mistura", NK_MAXIMIZED)) {
            section_shapes();
            nk_tree_pop(ctx);
        }
        if (nk_tree_push(ctx, NK_TREE_TAB, "Deformações (Barr)", NK_MINIMIZED)) {
            section_deform();
            nk_tree_pop(ctx);
        }
        if (nk_tree_push(ctx, NK_TREE_TAB, "Textura", NK_MINIMIZED)) {
            section_texture();
            nk_tree_pop(ctx);
        }
        if (nk_tree_push(ctx, NK_TREE_TAB, "Visualização", NK_MINIMIZED)) {
            section_view();
            nk_tree_pop(ctx);
        }
        if (nk_tree_push(ctx, NK_TREE_TAB, "Atalhos de teclado", NK_MAXIMIZED)) {
            section_help();
            nk_tree_pop(ctx);
        }
        section_bottom();
    }
    panel_open = !nk_window_is_collapsed(ctx, PANEL_NAME);
    nk_end(ctx);
}

void ui_render(void) {
    /* O backend liga blend/scissor e desliga o teste de profundidade. */
    nk_glfw3_render(&nk_glfw_state, NK_ANTI_ALIASING_ON, MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);
}
