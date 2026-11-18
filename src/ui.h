/* ui.h - Painel de controle (Nuklear) do Laboratório de Deformação. */
#ifndef UI_H
#define UI_H

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

int ui_init(GLFWwindow *win); /* retorna 0 em caso de erro */
void ui_shutdown(void);

void ui_frame(void);  /* lê a entrada e monta o painel (altera 'app') */
void ui_render(void); /* desenha o painel por cima da cena */

/* Largura ocupada pelo painel, em coordenadas da JANELA (0 se recolhido). */
float ui_panel_width(void);

/* O mouse está sobre o painel? (então não orbita a câmera nem dá zoom) */
int ui_wants_mouse(void);
/* Um campo de texto está sendo editado? (então os atalhos não disparam) */
int ui_wants_keyboard(void);

/* Presets de um clique (os mesmos botões do painel). */
#define UI_PRESET_COUNT 6
const char *ui_preset_name(int id);
void ui_apply_preset(int id); /* id em [0, UI_PRESET_COUNT) */

/* Eventos repassados pelos callbacks do main.c. */
void ui_on_key(GLFWwindow *win, int key, int scancode, int action, int mods);
void ui_on_char(GLFWwindow *win, unsigned int codepoint);
void ui_on_scroll(GLFWwindow *win, double dx, double dy);
void ui_on_mouse_button(GLFWwindow *win, int button, int action, int mods);

#endif /* UI_H */
