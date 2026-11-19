/* shader.c - Carregamento e compilação de shaders GLSL. */
#include "render/shader.h"

#include <stdio.h>
#include <stdlib.h>

/* Lê o arquivo inteiro para uma string alocada com malloc (ou NULL). */
static char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    char *buf = NULL;
    long len;
    if (!f) {
        fprintf(stderr, "Erro: nao foi possivel abrir '%s'\n", path);
        return NULL;
    }
    if (fseek(f, 0, SEEK_END) == 0 && (len = ftell(f)) >= 0 && fseek(f, 0, SEEK_SET) == 0) {
        buf = (char *)malloc((size_t)len + 1);
        if (buf) {
            size_t n = fread(buf, 1, (size_t)len, f);
            buf[n] = '\0';
        }
    }
    fclose(f);
    if (!buf) fprintf(stderr, "Erro: falha ao ler '%s'\n", path);
    return buf;
}

/* Compila um shader; retorna 0 em caso de erro (com o log no terminal). */
static GLuint compile(GLenum type, const char *path) {
    GLuint sh;
    GLint ok = 0;
    char *src = read_file(path);
    const char *csrc = src;
    if (!src) return 0;
    sh = glCreateShader(type);
    glShaderSource(sh, 1, &csrc, NULL);
    glCompileShader(sh);
    free(src);
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048];
        glGetShaderInfoLog(sh, sizeof(log), NULL, log);
        fprintf(stderr, "Erro compilando %s:\n%s\n", path, log);
        glDeleteShader(sh);
        return 0;
    }
    return sh;
}

void shader_find(const char *file_name, char *out, size_t out_size) {
    static const char *dirs[] = {
        "shaders/",
        "../shaders/",
#ifdef SHADER_DIR
        SHADER_DIR "/",
#endif
    };
    size_t i;
    for (i = 0; i < sizeof(dirs) / sizeof(dirs[0]); ++i) {
        FILE *f;
        snprintf(out, out_size, "%s%s", dirs[i], file_name);
        f = fopen(out, "rb");
        if (f) {
            fclose(f);
            return;
        }
    }
    snprintf(out, out_size, "shaders/%s", file_name); /* read_file mostrará o erro */
}

GLuint shader_load_program(const char *vert_path, const char *frag_path) {
    GLuint vs, fs, prog;
    GLint ok = 0;
    vs = compile(GL_VERTEX_SHADER, vert_path);
    fs = compile(GL_FRAGMENT_SHADER, frag_path);
    if (!vs || !fs) {
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        return 0;
    }
    prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs); /* continuam vivos enquanto anexados ao programa */
    glDeleteShader(fs);
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[2048];
        glGetProgramInfoLog(prog, sizeof(log), NULL, log);
        fprintf(stderr, "Erro linkando shaders:\n%s\n", log);
        glDeleteProgram(prog);
        return 0;
    }
    return prog;
}
