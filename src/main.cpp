// main.cpp - Morphing 3D em OpenGL moderno (3.3 core).
//
// Projeto Final - Computação Gráfica - UDESC
// Tema: Alteração de geometrias (morphing de objetos tridimensionais)
//
// Funcionamento:
//   1. Todas as formas são geradas na mesma grade paramétrica (shapes.cpp),
//      então existe correspondência 1:1 entre os vértices.
//   2. Cada forma fica em seu próprio VBO na GPU (enviado uma única vez).
//   3. O VAO liga a forma de ORIGEM nos atributos 0/1 e a de DESTINO
//      nos atributos 2/3. Trocar o par = só religar ponteiros, sem reenviar dados.
//   4. O vertex shader interpola posição e normal com o fator t.
#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cstdio>
#include <vector>

#include "mathutil.h"
#include "shader.h"
#include "shapes.h"

// ----------------------------------------------------------------------------
// Parâmetros da animação
// ----------------------------------------------------------------------------
const float MORPH_DURATION = 2.5f;  // segundos para ir de uma forma à outra
const float HOLD_TIME = 1.0f;       // pausa em cada forma no modo automático
const float MANUAL_SPEED = 0.5f;    // velocidade de t com as setas (por segundo)

// ----------------------------------------------------------------------------
// Estado do programa
// ----------------------------------------------------------------------------
struct MorphState {
    int from = 0;          // índice da forma de origem
    int to = 1;            // índice da forma de destino
    float t = 0.0f;        // progresso linear em [0,1]
    float hold = HOLD_TIME;
    bool autoPlay = true;
    bool smooth = true;    // usa curva de suavização (smoothstep)
    int queued = -1;       // destino escolhido pelo usuário durante um morph
};

struct Camera {
    float yaw = 0.6f, pitch = 0.35f, dist = 4.0f;
    bool dragging = false;
    double lastX = 0, lastY = 0;
};

enum RenderMode { SOLID = 0, SOLID_WIRE, WIRE, RENDER_MODE_COUNT };

struct App {
    std::vector<Shape> shapes;
    std::vector<GLuint> vbos;  // um VBO por forma (posição + normal intercaladas)
    GLuint vao = 0, ebo = 0, program = 0;
    GLsizei indexCount = 0;
    MorphState morph;
    Camera cam;
    int renderMode = SOLID;
    bool rotate = true;
    float objAngle = 0.0f;
};

App app;

// ----------------------------------------------------------------------------
// Funções auxiliares
// ----------------------------------------------------------------------------

// Curva de suavização: começa e termina devagar (derivada zero nas pontas).
float smoothstep01(float x) { return x * x * (3.0f - 2.0f * x); }

float easedT() { return app.morph.smooth ? smoothstep01(app.morph.t) : app.morph.t; }

Vec3 lerp(const Vec3& a, const Vec3& b, float t) { return a * (1.0f - t) + b * t; }

// Liga a forma "from" nos atributos 0/1 e a forma "to" nos atributos 2/3.
void bindShapePair(int from, int to) {
    const GLsizei stride = 6 * sizeof(float);
    glBindVertexArray(app.vao);

    glBindBuffer(GL_ARRAY_BUFFER, app.vbos[from]);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));

    glBindBuffer(GL_ARRAY_BUFFER, app.vbos[to]);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));

    glBindVertexArray(0);
}

// Envia todas as formas para a GPU e configura o VAO.
void setupGeometry() {
    app.shapes = buildShapes();
    std::vector<unsigned int> indices = buildGridIndices();
    app.indexCount = (GLsizei)indices.size();

    glGenVertexArrays(1, &app.vao);
    glBindVertexArray(app.vao);

    // Índices: a topologia é a mesma para todas as formas.
    glGenBuffers(1, &app.ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
                 indices.data(), GL_STATIC_DRAW);

    // Um VBO por forma, com [px py pz nx ny nz] por vértice.
    app.vbos.resize(app.shapes.size());
    glGenBuffers((GLsizei)app.vbos.size(), app.vbos.data());
    for (size_t s = 0; s < app.shapes.size(); ++s) {
        const Shape& sh = app.shapes[s];
        std::vector<float> data;
        data.reserve(sh.positions.size() * 6);
        for (size_t i = 0; i < sh.positions.size(); ++i) {
            const Vec3& p = sh.positions[i];
            const Vec3& n = sh.normals[i];
            data.insert(data.end(), {p.x, p.y, p.z, n.x, n.y, n.z});
        }
        glBindBuffer(GL_ARRAY_BUFFER, app.vbos[s]);
        glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_STATIC_DRAW);
    }

    for (GLuint a = 0; a < 4; ++a) glEnableVertexAttribArray(a);
    glBindVertexArray(0);

    bindShapePair(app.morph.from, app.morph.to);
}

// Avança para o próximo par de formas quando um morph termina.
void finishMorph() {
    MorphState& m = app.morph;
    m.from = m.to;
    if (m.queued >= 0 && m.queued != m.from) m.to = m.queued;
    else m.to = (m.to + 1) % (int)app.shapes.size();
    m.queued = -1;
    m.t = 0.0f;
    m.hold = HOLD_TIME;
    bindShapePair(m.from, m.to);
}

// Atualiza o estado da animação (dt em segundos).
void update(GLFWwindow* win, float dt) {
    MorphState& m = app.morph;

    if (m.autoPlay) {
        if (m.hold > 0.0f) m.hold -= dt;
        else m.t += dt / MORPH_DURATION;
    }
    // Controle manual com as setas (funciona com ou sem pausa).
    if (glfwGetKey(win, GLFW_KEY_RIGHT) == GLFW_PRESS) m.t += MANUAL_SPEED * dt;
    if (glfwGetKey(win, GLFW_KEY_LEFT) == GLFW_PRESS) m.t -= MANUAL_SPEED * dt;
    m.t = std::max(m.t, 0.0f);

    if (m.t >= 1.0f) finishMorph();

    if (app.rotate) app.objAngle += 0.4f * dt;
}

void render(int width, int height) {
    glViewport(0, 0, width, height);
    glClearColor(0.08f, 0.09f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (width == 0 || height == 0) return;

    const Camera& c = app.cam;
    Vec3 eye(c.dist * std::cos(c.pitch) * std::sin(c.yaw), c.dist * std::sin(c.pitch),
             c.dist * std::cos(c.pitch) * std::cos(c.yaw));
    Mat4 view = lookAt(eye, {0, 0, 0}, {0, 1, 0});
    Mat4 proj = perspective(0.8f, (float)width / height, 0.1f, 100.0f);
    Mat4 model = rotateY(app.objAngle);

    float t = easedT();
    const MorphState& m = app.morph;
    Vec3 color = lerp(app.shapes[m.from].color, app.shapes[m.to].color, t);

    glUseProgram(app.program);
    glUniformMatrix4fv(glGetUniformLocation(app.program, "uModel"), 1, GL_FALSE, model.m);
    glUniformMatrix4fv(glGetUniformLocation(app.program, "uView"), 1, GL_FALSE, view.m);
    glUniformMatrix4fv(glGetUniformLocation(app.program, "uProj"), 1, GL_FALSE, proj.m);
    glUniform1f(glGetUniformLocation(app.program, "uT"), t);
    glUniform3f(glGetUniformLocation(app.program, "uColor"), color.x, color.y, color.z);
    glUniform3f(glGetUniformLocation(app.program, "uLightPos"), 3.0f, 4.0f, 3.0f);
    glUniform3f(glGetUniformLocation(app.program, "uEyePos"), eye.x, eye.y, eye.z);
    GLint wireLoc = glGetUniformLocation(app.program, "uWireframe");

    glBindVertexArray(app.vao);

    if (app.renderMode == SOLID || app.renderMode == SOLID_WIRE) {
        // Empurra o sólido um pouco para trás para a malha aparecer por cima.
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1.0f, 1.0f);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glUniform1i(wireLoc, 0);
        glDrawElements(GL_TRIANGLES, app.indexCount, GL_UNSIGNED_INT, 0);
        glDisable(GL_POLYGON_OFFSET_FILL);
    }
    if (app.renderMode == SOLID_WIRE || app.renderMode == WIRE) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glUniform1i(wireLoc, 1);
        // Linhas escuras sobre o sólido; claras quando só a malha aparece.
        Vec3 wc = (app.renderMode == WIRE) ? color : color * 0.35f;
        glUniform3f(glGetUniformLocation(app.program, "uWireColor"), wc.x, wc.y, wc.z);
        glDrawElements(GL_TRIANGLES, app.indexCount, GL_UNSIGNED_INT, 0);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    glBindVertexArray(0);
}

void updateTitle(GLFWwindow* win, float fps) {
    static const char* modeNames[] = {"solido", "solido+malha", "malha"};
    const MorphState& m = app.morph;
    char title[256];
    std::snprintf(title, sizeof(title),
                  "Morphing 3D | %s -> %s | t = %.2f | %s | curva: %s | %s | %.0f FPS",
                  app.shapes[m.from].name.c_str(), app.shapes[m.to].name.c_str(), easedT(),
                  m.autoPlay ? "automatico" : "pausado", m.smooth ? "smoothstep" : "linear",
                  modeNames[app.renderMode], fps);
    glfwSetWindowTitle(win, title);
}

void printHelp() {
    std::printf(
        "\n=== Morphing 3D - Controles ===\n"
        "  ESPACO        pausa / retoma a animacao automatica\n"
        "  SETAS < >     controla o morph manualmente (t diminui / aumenta)\n"
        "  1 a %zu         escolhe a proxima forma de destino\n"
        "  S             alterna curva linear / suavizada (smoothstep)\n"
        "  W             alterna solido / solido+malha / so malha\n"
        "  R             liga / desliga a rotacao do objeto\n"
        "  Mouse (esq.)  arrasta para orbitar a camera\n"
        "  Scroll        zoom\n"
        "  ESC           sair\n\n"
        "Formas:",
        app.shapes.size());
    for (size_t i = 0; i < app.shapes.size(); ++i)
        std::printf("  [%zu] %s", i + 1, app.shapes[i].name.c_str());
    std::printf("\n\n");
}

// ----------------------------------------------------------------------------
// Callbacks de entrada
// ----------------------------------------------------------------------------
void keyCallback(GLFWwindow* win, int key, int, int action, int) {
    if (action != GLFW_PRESS) return;
    MorphState& m = app.morph;
    switch (key) {
        case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(win, GLFW_TRUE); break;
        case GLFW_KEY_SPACE:  m.autoPlay = !m.autoPlay; break;
        case GLFW_KEY_S:      m.smooth = !m.smooth; break;
        case GLFW_KEY_W:      app.renderMode = (app.renderMode + 1) % RENDER_MODE_COUNT; break;
        case GLFW_KEY_R:      app.rotate = !app.rotate; break;
        default:
            if (key >= GLFW_KEY_1 && key < GLFW_KEY_1 + (int)app.shapes.size()) {
                int target = key - GLFW_KEY_1;
                if (target == m.from) break;
                if (m.t == 0.0f) {
                    // Ainda não começou a deformar: troca o destino na hora.
                    m.to = target;
                    m.hold = 0.0f;
                    bindShapePair(m.from, m.to);
                } else {
                    // No meio de um morph: o destino escolhido vem em seguida.
                    m.queued = target;
                }
            }
    }
}

void mouseButtonCallback(GLFWwindow* win, int button, int action, int) {
    if (button != GLFW_MOUSE_BUTTON_LEFT) return;
    app.cam.dragging = (action == GLFW_PRESS);
    glfwGetCursorPos(win, &app.cam.lastX, &app.cam.lastY);
}

void cursorCallback(GLFWwindow*, double x, double y) {
    Camera& c = app.cam;
    if (!c.dragging) return;
    c.yaw -= (float)(x - c.lastX) * 0.01f;
    c.pitch += (float)(y - c.lastY) * 0.01f;
    c.pitch = std::clamp(c.pitch, -1.5f, 1.5f);
    c.lastX = x;
    c.lastY = y;
}

void scrollCallback(GLFWwindow*, double, double dy) {
    app.cam.dist = std::clamp(app.cam.dist - (float)dy * 0.3f, 1.5f, 15.0f);
}

// ----------------------------------------------------------------------------
int main() {
    if (!glfwInit()) {
        std::fprintf(stderr, "Erro: falha ao iniciar o GLFW\n");
        return 1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);  // necessário no macOS
    glfwWindowHint(GLFW_SAMPLES, 4);                        // antialiasing

    GLFWwindow* win = glfwCreateWindow(1024, 720, "Morphing 3D", nullptr, nullptr);
    if (!win) {
        std::fprintf(stderr, "Erro: nao foi possivel criar janela OpenGL 3.3\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(win);
    glfwSwapInterval(1);  // vsync

    if (!gladLoadGL(glfwGetProcAddress)) {
        std::fprintf(stderr, "Erro: falha ao carregar funcoes OpenGL (GLAD)\n");
        return 1;
    }
    std::printf("OpenGL %s | %s\n", glGetString(GL_VERSION), glGetString(GL_RENDERER));

    glfwSetKeyCallback(win, keyCallback);
    glfwSetMouseButtonCallback(win, mouseButtonCallback);
    glfwSetCursorPosCallback(win, cursorCallback);
    glfwSetScrollCallback(win, scrollCallback);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);

    app.program = loadShaderProgram(findShader("morph.vert"), findShader("morph.frag"));
    setupGeometry();
    printHelp();

    double last = glfwGetTime(), titleTimer = 0.0;
    int frames = 0;
    while (!glfwWindowShouldClose(win)) {
        double now = glfwGetTime();
        float dt = (float)(now - last);
        last = now;

        update(win, dt);

        int w, h;
        glfwGetFramebufferSize(win, &w, &h);
        render(w, h);
        glfwSwapBuffers(win);
        glfwPollEvents();

        // Atualiza o título (estado + FPS) 4 vezes por segundo.
        ++frames;
        titleTimer += dt;
        if (titleTimer >= 0.25) {
            updateTitle(win, frames / (float)titleTimer);
            frames = 0;
            titleTimer = 0.0;
        }
    }

    glDeleteBuffers((GLsizei)app.vbos.size(), app.vbos.data());
    glDeleteBuffers(1, &app.ebo);
    glDeleteVertexArrays(1, &app.vao);
    glDeleteProgram(app.program);
    glfwTerminate();
    return 0;
}
