#version 330 core
// Vertex shader: só posiciona a forma na tela (câmera e projeção).
//
// O morph NÃO acontece aqui: a forma intermediária já chega pronta,
// calculada em C (src/geometry/morph.c) a cada quadro.
layout(location = 0) in vec3 aPos;     // posição do vértice (já misturada)
layout(location = 1) in vec3 aNormal;  // normal do vértice (já misturada)

uniform mat4 uModel;  // rotação do objeto (tecla R)
uniform mat4 uView;   // câmera
uniform mat4 uProj;   // perspectiva

out vec3 vWorldPos;   // posição no mundo (para a iluminação)
out vec3 vNormal;     // normal no mundo (para a iluminação)

void main() {
    vec4 world = uModel * vec4(aPos, 1.0);
    vWorldPos = world.xyz;
    // uModel só tem rotação, então serve também para girar as normais.
    vNormal = mat3(uModel) * aNormal;
    gl_Position = uProj * uView * world;
}
