// shader.h - Carregamento e compilação de shaders GLSL.
#pragma once
#include <glad/gl.h>
#include <string>

// Lê os arquivos, compila e linka o programa. Encerra o programa com
// mensagem de erro se algo falhar.
GLuint loadShaderProgram(const std::string& vertPath, const std::string& fragPath);

// Procura um arquivo de shader em alguns diretórios comuns
// (./shaders, ../shaders e o diretório definido na compilação).
std::string findShader(const std::string& fileName);
