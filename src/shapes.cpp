// shapes.cpp - Formas paramétricas sobre a mesma grade (u, v).
//
// Convenção da grade, para todas as formas:
//   u em [0,1] -> ângulo theta = 2*pi*u em torno do eixo Y
//   v em [0,1] -> vai do "topo" (v=0) até a "base" (v=1)
// Pontos com o mesmo (u, v) em formas diferentes são correspondentes.
#include "shapes.h"
#include <algorithm>
#include <cmath>

namespace {

const float PI = 3.14159265358979f;

// Direção na esfera unitária para o par (u, v). Serve de base para esfera e cubo.
Vec3 sphereDir(float u, float v) {
    float theta = 2.0f * PI * u;
    float phi = PI * v;  // 0 = polo norte, pi = polo sul
    return {std::sin(phi) * std::cos(theta), std::cos(phi), std::sin(phi) * std::sin(theta)};
}

// Uma função por forma: recebe (u, v) e devolve posição e normal.
using ShapeFn = void (*)(float u, float v, Vec3& pos, Vec3& nrm);

void sphere(float u, float v, Vec3& p, Vec3& n) {
    n = sphereDir(u, v);
    p = n * 1.0f;
}

// Faixa de v ocupada por cada tampa (cubo e cilindro). Com GRID_V
// múltiplo de 4, a borda das tampas cai exatamente numa linha da grade.
const float CAP = 0.25f;

// Cubo: mesma estrutura do cilindro (tampa / lateral / tampa), mas o
// contorno é um quadrado em vez de um círculo. A direção (cos, sin) é
// projetada no quadrado; os cantos ficam em theta = 45°, 135°... que
// coincidem com colunas da grade (GRID_U múltiplo de 8). Assim as
// arestas do cubo ficam alinhadas com as arestas da malha (sem serrilhado).
void cube(float u, float v, Vec3& p, Vec3& n) {
    const float h = 0.8f;  // meia aresta
    float theta = 2.0f * PI * u;
    float c = std::cos(theta), s = std::sin(theta);
    float k = h / std::max(std::fabs(c), std::fabs(s));  // raio até o quadrado
    if (v < CAP) {
        float r = k * (v / CAP);
        p = {r * c, h, r * s};
    } else if (v > 1.0f - CAP) {
        float r = k * ((1.0f - v) / CAP);
        p = {r * c, -h, r * s};
    } else {
        float t = (v - CAP) / (1.0f - 2.0f * CAP);
        p = {k * c, h - 2.0f * h * t, k * s};
    }
    // Normal: soma das normais das faces em que o ponto está. Nas arestas
    // e cantos isso dá a média das faces vizinhas.
    const float eps = 1e-4f;
    n = {std::fabs(p.x) > h - eps ? (p.x > 0 ? 1.0f : -1.0f) : 0.0f,
         std::fabs(p.y) > h - eps ? (p.y > 0 ? 1.0f : -1.0f) : 0.0f,
         std::fabs(p.z) > h - eps ? (p.z > 0 ? 1.0f : -1.0f) : 0.0f};
}

// Toro: theta percorre o anel maior, phi percorre o tubo.
// phi = pi - 2*pi*v faz o "equador" da esfera (v=0.5) cair na borda
// externa do toro e os polos (v=0 e v=1) caírem na borda interna.
void torus(float u, float v, Vec3& p, Vec3& n) {
    const float R = 0.75f, r = 0.32f;
    float theta = 2.0f * PI * u;
    float phi = PI - 2.0f * PI * v;
    float ring = R + r * std::cos(phi);
    p = {ring * std::cos(theta), r * std::sin(phi), ring * std::sin(theta)};
    n = {std::cos(phi) * std::cos(theta), std::sin(phi), std::cos(phi) * std::sin(theta)};
}

// Cilindro com tampas: a faixa v é dividida em tampa superior,
// lateral e tampa inferior.
void cylinder(float u, float v, Vec3& p, Vec3& n) {
    const float radius = 0.75f, h = 0.9f, cap = CAP;
    float theta = 2.0f * PI * u;
    float c = std::cos(theta), s = std::sin(theta);
    if (v < cap) {                       // tampa de cima
        float k = radius * (v / cap);
        p = {k * c, h, k * s};
        n = {0, 1, 0};
    } else if (v > 1.0f - cap) {         // tampa de baixo
        float k = radius * ((1.0f - v) / cap);
        p = {k * c, -h, k * s};
        n = {0, -1, 0};
    } else {                             // lateral
        float t = (v - cap) / (1.0f - 2.0f * cap);
        p = {radius * c, h - 2.0f * h * t, radius * s};
        n = {c, 0, s};
    }
}

Shape sample(const char* name, ShapeFn fn, Vec3 color) {
    Shape s;
    s.name = name;
    s.color = color;
    for (int j = 0; j <= GRID_V; ++j) {
        for (int i = 0; i <= GRID_U; ++i) {
            Vec3 p, n;
            fn(float(i) / GRID_U, float(j) / GRID_V, p, n);
            s.positions.push_back(p);
            s.normals.push_back(normalize(n));
        }
    }
    return s;
}

}  // namespace

std::vector<Shape> buildShapes() {
    return {
        sample("Esfera",   sphere,   {0.25f, 0.55f, 0.95f}),
        sample("Cubo",     cube,     {0.95f, 0.55f, 0.20f}),
        sample("Toro",     torus,    {0.30f, 0.80f, 0.40f}),
        sample("Cilindro", cylinder, {0.70f, 0.40f, 0.90f}),
    };
}

std::vector<unsigned int> buildGridIndices() {
    std::vector<unsigned int> idx;
    const unsigned int row = GRID_U + 1;
    for (unsigned int j = 0; j < GRID_V; ++j) {
        for (unsigned int i = 0; i < GRID_U; ++i) {
            unsigned int a = j * row + i, b = a + 1;
            unsigned int c = a + row, d = c + 1;
            idx.insert(idx.end(), {a, c, b, b, c, d});
        }
    }
    return idx;
}
