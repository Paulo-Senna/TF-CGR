/* file_dialog.h - Janelas do sistema para escolher um arquivo (abrir/salvar).
 *
 * Windows: diálogo nativo (GetOpenFileName/GetSaveFileName).
 * Linux:   chama o programa 'zenity' (GNOME) ou 'kdialog' (KDE), que já
 *          vêm instalados na maioria das distribuições.
 * O programa fica parado enquanto a janela está aberta (ela é modal). */
#ifndef FILE_DIALOG_H
#define FILE_DIALOG_H

#include <stddef.h>

enum { DIALOG_CANCELLED = 0, DIALOG_OK = 1, DIALOG_UNAVAILABLE = -1 };

/* 'title' e 'filter_name' devem ser ASCII (o Windows não lê UTF-8 aqui).
 * 'patterns' é uma lista separada por espaços, ex.: "*.png *.jpg".
 * O caminho escolhido vai para 'out'. Retorna um dos DIALOG_*. */
int dialog_open_file(const char *title, const char *filter_name, const char *patterns,
                     char *out, size_t out_size);

/* Igual, para salvar: 'default_name' é o nome sugerido (ex.: "modelo.obj"). */
int dialog_save_file(const char *title, const char *default_name, const char *filter_name,
                     const char *patterns, char *out, size_t out_size);

#endif /* FILE_DIALOG_H */
