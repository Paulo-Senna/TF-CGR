/* file_dialog.c - Janelas de abrir/salvar arquivo (ver file_dialog.h). */
#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L /* popen/pclose não fazem parte do C99 puro */
#endif

#include "io/file_dialog.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
/* ------------------------------------------------------------------ Windows */
#include <windows.h>
#include <commdlg.h>

/* Filtro no formato do Windows: "Nome\0*.a;*.b\0Todos\0*.*\0\0". */
static void build_filter(char *buf, size_t size, const char *name, const char *patterns) {
    size_t n = 0, i;
    n += (size_t)snprintf(buf + n, size - n, "%s", name) + 1;
    for (i = 0; patterns[i] && n + 2 < size; ++i) buf[n++] = patterns[i] == ' ' ? ';' : patterns[i];
    buf[n++] = '\0';
    n += (size_t)snprintf(buf + n, size - n, "Todos os arquivos") + 1;
    n += (size_t)snprintf(buf + n, size - n, "*.*") + 1;
    buf[n] = '\0';
}

static int run(int save, const char *title, const char *default_name, const char *filter_name,
               const char *patterns, char *out, size_t out_size) {
    OPENFILENAMEA ofn;
    char filter[512];
    const char *ext = default_name ? strrchr(default_name, '.') : NULL;
    BOOL ok;
    build_filter(filter, sizeof(filter), filter_name, patterns);
    snprintf(out, out_size, "%s", default_name ? default_name : "");
    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = out;
    ofn.nMaxFile = (DWORD)out_size;
    ofn.lpstrTitle = title;
    ofn.lpstrDefExt = ext ? ext + 1 : NULL;
    ofn.Flags = OFN_NOCHANGEDIR | (save ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST);
    ok = save ? GetSaveFileNameA(&ofn) : GetOpenFileNameA(&ofn);
    return ok ? DIALOG_OK : DIALOG_CANCELLED;
}

#else
/* ------------------------------------------------------------ Linux (e Unix) */

static int has_program(const char *name) {
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "command -v %s >/dev/null 2>&1", name);
    return system(cmd) == 0;
}

/* Roda o comando e lê a primeira linha que ele imprime (o caminho). */
static int read_command(const char *cmd, char *out, size_t out_size) {
    FILE *p = popen(cmd, "r");
    size_t len;
    out[0] = '\0';
    if (!p) return DIALOG_UNAVAILABLE;
    if (!fgets(out, (int)out_size, p)) out[0] = '\0';
    pclose(p);
    len = strlen(out);
    while (len > 0 && (out[len - 1] == '\n' || out[len - 1] == '\r')) out[--len] = '\0';
    return len > 0 ? DIALOG_OK : DIALOG_CANCELLED; /* nada impresso = cancelou */
}

/* Os textos vêm deste programa (sem aspas simples), então podem ir entre
 * aspas simples no comando do shell sem risco. */
static int run(int save, const char *title, const char *default_name, const char *filter_name,
               const char *patterns, char *out, size_t out_size) {
    char cmd[1024];
    if (has_program("zenity")) {
        snprintf(cmd, sizeof(cmd),
                 "zenity --file-selection %s --title='%s' %s%s%s --file-filter='%s | %s' "
                 "--file-filter='Todos os arquivos | *' 2>/dev/null",
                 save ? "--save" : "", title, save ? "--filename='" : "",
                 save ? default_name : "", save ? "'" : "", filter_name, patterns);
    } else if (has_program("kdialog")) {
        snprintf(cmd, sizeof(cmd), "kdialog --title '%s' %s '%s' '%s|%s' 2>/dev/null", title,
                 save ? "--getsavefilename" : "--getopenfilename", save ? default_name : ".",
                 patterns, filter_name);
    } else {
        return DIALOG_UNAVAILABLE;
    }
    return read_command(cmd, out, out_size);
}
#endif

int dialog_open_file(const char *title, const char *filter_name, const char *patterns,
                     char *out, size_t out_size) {
    return run(0, title, NULL, filter_name, patterns, out, out_size);
}

int dialog_save_file(const char *title, const char *default_name, const char *filter_name,
                     const char *patterns, char *out, size_t out_size) {
    return run(1, title, default_name, filter_name, patterns, out, out_size);
}
