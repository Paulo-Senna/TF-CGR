#version 330 core
// Vertex shader do morphing.
// Cada vértice recebe a posição/normal da forma de ORIGEM (A) e da forma
// de DESTINO (B). A interpolação é feita aqui, na GPU:
//     P(t) = (1 - t) * A + t * B
layout(location = 0) in vec3 aPosA;
layout(location = 1) in vec3 aNrmA;
layout(location = 2) in vec3 aPosB;
layout(location = 3) in vec3 aNrmB;

uniform float uT;          // fator de interpolação em [0,1]
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

out vec3 vWorldPos;
out vec3 vNormal;

void main() {
    vec3 pos = mix(aPosA, aPosB, uT);
    vec3 nrm = normalize(mix(aNrmA, aNrmB, uT));

    vec4 world = uModel * vec4(pos, 1.0);
    vWorldPos = world.xyz;
    // uModel só tem rotação, então serve também para as normais.
    vNormal = mat3(uModel) * nrm;
    gl_Position = uProj * uView * world;
}
