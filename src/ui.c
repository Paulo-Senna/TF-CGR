/* ui.c - Painel de controle (Nuklear) do Laboratório de Deformação.
 *
 * O Nuklear é uma interface "modo imediato": a cada quadro o painel
 * inteiro é descrito de novo, lendo e escrevendo direto no estado 'app'.
 * Não existe "objeto botão" guardado; um botão é só um if. */
#include "ui.h"

#include <stdio.h>

#include "app.h"
#include "nuklear_setup.h"

#define PANEL_NAME "painel"
#define PANEL_TITLE "Laboratório de Deformação"
#define PANEL_WIDTH 330.0f
#define ROW_H 20.0f

/* Tamanho máximo dos buffers de vértices/índices do painel na GPU. */
#define MAX_VERTEX_BUFFER (512 * 1024)
#define MAX_ELEMENT_BUFFER (128 * 1024)

static struct nk_glfw nk_glfw_state; /* estado do backend GLFW + OpenGL 3 */
static struct nk_context *ctx;
static int panel_open = 1;  /* painel expandido (não minimizado)? */
static int text_active = 0; /* algum campo de texto em edição neste quadro? */

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
int ui_wants_keyboard(void) { return text_active; }
float ui_panel_width(void) { return panel_open ? PANEL_WIDTH : 0.0f; }

/* ----------------------------------------------------------------------------
 * Presets de um clique: configuram formas, mistura, deformações e textura
 * de uma vez, para a demonstração.
 * ------------------------------------------------------------------------- */
typedef struct {
    const char *label;  /* texto do botão (curto, cabe em meia linha) */
    const char *hint;   /* descrição completa (dica ao passar o mouse) */
    int from, to;       /* formas A e B */
    int auto_play;      /* 1 = anima de A para B (e segue o ciclo) */
    float taper, twist, bend;
    int texture;
    float tex_scale;
    int tex_multiply;
} Preset;

static const Preset presets[UI_PRESET_COUNT - 1] = {
    {"Esfera torcida", "Esfera xadrez torcida: a esfera é simétrica em Y, só a textura revela o twist",
     SHAPE_SPHERE, SHAPE_SPHERE, 0, 0.0f, 3.0f, 0.0f, TEX_CHECKER, 2.0f, 1},
    {"Cubo -> Toro", "Cubo -> Toro com listras (morph automático)",
     SHAPE_CUBE, SHAPE_TORUS, 1, 0.0f, 0.0f, 0.0f, TEX_STRIPES, 1.0f, 1},
    {"Cilindro dobrado", "Cilindro dobrado (bend = 1) com grade UV",
     SHAPE_CYLINDER, SHAPE_CYLINDER, 0, 0.0f, 0.0f, 1.0f, TEX_UV_GRID, 1.0f, 0},
    {"Cone por taper", "Cilindro afunilado (taper = -0.8) com xadrez",
     SHAPE_CYLINDER, SHAPE_CYLINDER, 0, -0.8f, 0.0f, 0.0f, TEX_CHECKER, 1.0f, 1},
    {"Plano -> Esfera", "Plano vira esfera (morph automático) com grade UV",
     SHAPE_PLANE, SHAPE_SPHERE, 1, 0.0f, 0.0f, 0.0f, TEX_UV_GRID, 1.0f, 0},
};

const char *ui_preset_name(int id) {
    if (id == UI_PRESET_COUNT - 1) return "Resetar tudo";
    return (id >= 0 && id < UI_PRESET_COUNT - 1) ? presets[id].hint : "?";
}

void ui_apply_preset(int id) {
    const Preset *p;
    if (id == UI_PRESET_COUNT - 1) { /* último botão: volta ao estado inicial */
        app_reset_state();
        app_bind_shape_pair();
        return;
    }
    if (id < 0 || id >= UI_PRESET_COUNT - 1) return;
    p = &presets[id];

    app_set_shapes(p->from, p->to);
    app.morph.t = 0.0f;
    app.morph.hold = HOLD_TIME;
    app.morph.auto_play = p->auto_play;
    app.morph.queued = -1; /* preset descarta destino que estava na fila */

    app.deform.taper = p->taper;
    app.deform.twist = p->twist;
    app.deform.bend = p->bend;

    app.tex.current = p->texture;
    app.tex.scale = p->tex_scale;
    app.tex.multiply = p->tex_multiply;
}

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
    const char *names[SHAPE_COUNT];
    int i;
    for (i = 0; i < SHAPE_COUNT; ++i) names[i] = app.shapes[i].name;
    nk_layout_row(ctx, NK_DYNAMIC, ROW_H, 2, ratio);
    nk_label(ctx, label, NK_TEXT_LEFT);
    return nk_combo(ctx, names, SHAPE_COUNT, selected, (int)ROW_H, nk_vec2(180, 160));
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
static void section_presets(void) {
    int i;
    nk_layout_row_dynamic(ctx, ROW_H + 2, 2);
    for (i = 0; i < UI_PRESET_COUNT; ++i) {
        const char *label = (i < UI_PRESET_COUNT - 1) ? presets[i].label : "Resetar tudo";
        if (nk_widget_is_hovered(ctx)) nk_tooltip(ctx, ui_preset_name(i));
        if (nk_button_label(ctx, label)) ui_apply_preset(i);
    }
}

static void section_shapes(void) {
    MorphState *m = &app.morph;
    int a, b, smooth = m->smooth;
    float t = m->t;
    char text[96];

    a = shape_combo_row("Forma A", m->from);
    b = shape_combo_row("Forma B", m->to);
    if (a != m->from || b != m->to) app_set_shapes(a, b);

    if (slider_row("t", &t, 0.0f, 1.0f, 0.001f)) app_set_t(t);

    nk_layout_row_dynamic(ctx, ROW_H, 1);
    snprintf(text, sizeof(text), "Mistura exibida: %.0f%% de %s", app_eased_t() * 100.0f,
             app.shapes[m->to].name);
    nk_label(ctx, text, NK_TEXT_LEFT);
    if (m->queued >= 0) {
        snprintf(text, sizeof(text), "Próximo destino: %s", app.shapes[m->queued].name);
        nk_label_colored(ctx, text, NK_TEXT_LEFT, nk_rgb(240, 200, 90));
    }

    checkbox_row("Animação automática (ESPAÇO)", &m->auto_play);
    if (checkbox_row("Curva suave - smoothstep (S)", &smooth)) app_set_smooth(smooth);
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
    static const float path_ratio[] = {0.72f, 0.28f};
    TexState *tx = &app.tex;
    const char *names[TEX_COUNT];
    nk_flags edit;
    int i, sel;

    for (i = 0; i < TEX_COUNT; ++i) names[i] = texture_name(i);
    nk_layout_row(ctx, NK_DYNAMIC, ROW_H, 2, ratio);
    nk_label(ctx, "Textura", NK_TEXT_LEFT);
    sel = nk_combo(ctx, names, TEX_COUNT, tx->current, (int)ROW_H, nk_vec2(180, 160));
    if (sel != tx->current) {
        if (sel == TEX_IMAGE && !tx->ids[TEX_IMAGE]) {
            snprintf(tx->message, sizeof(tx->message), "Nenhuma imagem carregada ainda.");
            tx->message_is_error = 1;
        } else {
            tx->current = sel;
        }
    }

    slider_row("Repetição", &tx->scale, TEX_SCALE_MIN, TEX_SCALE_MAX, 0.05f);
    checkbox_row("Multiplicar pela cor da forma", &tx->multiply);

    /* Caminho da imagem: Enter ou o botão carregam. Arrastar o arquivo
     * para a janela também funciona (drop_callback em main.c). */
    nk_layout_row(ctx, NK_DYNAMIC, ROW_H + 2, 2, path_ratio);
    edit = nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD | NK_EDIT_SIG_ENTER, tx->path,
                                          (int)sizeof(tx->path), nk_filter_default);
    if (edit & NK_EDIT_ACTIVE) text_active = 1;
    if (nk_widget_is_hovered(ctx))
        nk_tooltip(ctx, "Carrega o PNG/JPG do caminho ao lado (ou arraste o arquivo para a janela)");
    if (nk_button_label(ctx, "Carregar") || (edit & NK_EDIT_COMMITED)) {
        nk_edit_unfocus(ctx); /* libera o teclado para os atalhos de novo */
        app_load_image(tx->path);
    }

    if (tx->ids[TEX_IMAGE]) {
        char text[128];
        nk_layout_row_dynamic(ctx, 16, 1);
        snprintf(text, sizeof(text), "Imagem: %s", tx->image_info);
        nk_label_colored(ctx, text, NK_TEXT_LEFT, nk_rgb(150, 150, 150));
    }
    if (tx->message[0]) {
        nk_layout_row_dynamic(ctx, 34, 1);
        nk_label_colored_wrap(ctx, tx->message,
                              tx->message_is_error ? nk_rgb(255, 110, 100) : nk_rgb(130, 220, 130));
    }
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
        "ESPAÇO   pausa / retoma a animação",
        "<- ->    controla o morph (segurar)",
        "1 a 5    escolhe a próxima forma",
        "S        curva linear / smoothstep",
        "W        sólido / sólido+malha / malha",
        "R        rotação do objeto",
        "Mouse    arrastar orbita, scroll = zoom",
        "ESC      sair",
    };
    size_t i;
    nk_layout_row_dynamic(ctx, 16, 1);
    for (i = 0; i < sizeof(lines) / sizeof(lines[0]); ++i) nk_label(ctx, lines[i], NK_TEXT_LEFT);
}

/* ----------------------------------------------------------------------------
 * Quadro
 * ------------------------------------------------------------------------- */
void ui_frame(void) {
    int win_w, win_h;
    struct nk_rect bounds;
    const nk_flags flags = NK_WINDOW_BORDER | NK_WINDOW_TITLE | NK_WINDOW_MINIMIZABLE;

    text_active = 0;
    nk_glfw3_new_frame(&nk_glfw_state);

    /* Painel preso à esquerda, com a altura da janela. */
    glfwGetWindowSize(nk_glfw_state.win, &win_w, &win_h);
    bounds = nk_rect(0, 0, PANEL_WIDTH, (float)win_h);
    if (nk_window_find(ctx, PANEL_NAME)) nk_window_set_bounds(ctx, PANEL_NAME, bounds);

    if (nk_begin_titled(ctx, PANEL_NAME, PANEL_TITLE, bounds, flags)) {
        if (nk_tree_push(ctx, NK_TREE_TAB, "Presets (um clique)", NK_MAXIMIZED)) {
            section_presets();
            nk_tree_pop(ctx);
        }
        if (nk_tree_push(ctx, NK_TREE_TAB, "Forma e mistura", NK_MAXIMIZED)) {
            section_shapes();
            nk_tree_pop(ctx);
        }
        if (nk_tree_push(ctx, NK_TREE_TAB, "Deformações (Barr)", NK_MAXIMIZED)) {
            section_deform();
            nk_tree_pop(ctx);
        }
        if (nk_tree_push(ctx, NK_TREE_TAB, "Textura", NK_MAXIMIZED)) {
            section_texture();
            nk_tree_pop(ctx);
        }
        if (nk_tree_push(ctx, NK_TREE_TAB, "Visualização", NK_MAXIMIZED)) {
            section_view();
            nk_tree_pop(ctx);
        }
        if (nk_tree_push(ctx, NK_TREE_TAB, "Atalhos de teclado", NK_MINIMIZED)) {
            section_help();
            nk_tree_pop(ctx);
        }
    }
    panel_open = !nk_window_is_collapsed(ctx, PANEL_NAME);
    nk_end(ctx);
}

void ui_render(void) {
    /* O backend liga blend/scissor e desliga o teste de profundidade. */
    nk_glfw3_render(&nk_glfw_state, NK_ANTI_ALIASING_ON, MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);
}
