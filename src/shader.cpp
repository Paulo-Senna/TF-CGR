// shader.cpp - Carregamento e compilação de shaders GLSL.
#include "shader.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace {

std::string readFile(const std::string& path) {
    std::ifstream f(path);
    if (!f) {
        std::fprintf(stderr, "Erro: nao foi possivel abrir '%s'\n", path.c_str());
        std::exit(1);
    }
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

GLuint compile(GLenum type, const std::string& src, const std::string& path) {
    GLuint sh = glCreateShader(type);
    const char* c = src.c_str();
    glShaderSource(sh, 1, &c, nullptr);
    glCompileShader(sh);
    GLint ok = 0;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048];
        glGetShaderInfoLog(sh, sizeof(log), nullptr, log);
        std::fprintf(stderr, "Erro compilando %s:\n%s\n", path.c_str(), log);
        std::exit(1);
    }
    return sh;
}

}  // namespace

std::string findShader(const std::string& fileName) {
    const std::string dirs[] = {
        "shaders/",
        "../shaders/",
#ifdef SHADER_DIR
        SHADER_DIR "/",
#endif
    };
    for (const auto& d : dirs) {
        std::ifstream f(d + fileName);
        if (f) return d + fileName;
    }
    return "shaders/" + fileName;  // readFile mostrará o erro
}

GLuint loadShaderProgram(const std::string& vertPath, const std::string& fragPath) {
    GLuint vs = compile(GL_VERTEX_SHADER, readFile(vertPath), vertPath);
    GLuint fs = compile(GL_FRAGMENT_SHADER, readFile(fragPath), fragPath);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    GLint ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[2048];
        glGetProgramInfoLog(prog, sizeof(log), nullptr, log);
        std::fprintf(stderr, "Erro linkando shaders:\n%s\n", log);
        std::exit(1);
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}
