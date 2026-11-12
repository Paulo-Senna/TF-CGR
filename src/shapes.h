// shapes.h - Geração das formas usadas no morphing.
//
// Ideia central: TODAS as formas são amostradas sobre a MESMA grade
// paramétrica (u, v) com a mesma resolução. Assim o vértice i da esfera
// corresponde ao vértice i do cubo, do toro etc. (correspondência 1:1),
// e o morphing vira uma simples interpolação vértice a vértice.
#pragma once
#include <string>
#include <vector>
#include "mathutil.h"

struct Shape {
    std::string name;
    std::vector<Vec3> positions;  // um por vértice da grade
    std::vector<Vec3> normals;    // normal de cada vértice
    Vec3 color;                   // cor base da forma
};

// Resolução da grade paramétrica (iguais para todas as formas).
constexpr int GRID_U = 64;  // divisões em torno do eixo Y (longitude)
constexpr int GRID_V = 48;  // divisões de cima para baixo (latitude)
static_assert(GRID_U % 8 == 0 && GRID_V % 4 == 0,
              "arestas do cubo/cilindro precisam cair em linhas da grade");

// Cria a lista de formas (esfera, cubo, toro, cilindro, ...).
std::vector<Shape> buildShapes();

// Índices dos triângulos da grade (compartilhados por todas as formas).
std::vector<unsigned int> buildGridIndices();
