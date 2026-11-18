#version 330 core
// Vertex shader do Laboratório de Deformação.
//
// Etapa 1 - MISTURA (morphing): cada vértice recebe a posição/normal da
// forma A e da forma B (mesmo vértice da grade nas duas). A interpolação é
// feita aqui, na GPU:
//     P(t) = (1 - t) * A + t * B
//
// Etapa 2 - DEFORMAÇÕES DE BARR (1984), aplicadas DEPOIS da mistura, em
// espaço do objeto, ao longo do eixo Y, nesta ordem:
//     taper (afunilamento) -> twist (torção) -> bend (dobra)
layout(location = 0) in vec3 aPosA;
layout(location = 1) in vec3 aNrmA;
layout(location = 2) in vec3 aPosB;
layout(location = 3) in vec3 aNrmB;
layout(location = 4) in vec2 aUV;  // (u,v) da grade: igual para todas as formas

uniform float uT;      // fator de mistura em [0,1] (já com a curva aplicada)
uniform float uTaper;  // afunilamento: escala de x,z por unidade de y
uniform float uTwist;  // torção: radianos por unidade de y
uniform float uBend;   // dobra: curvatura (1/raio)
uniform float uTexScale;  // "Repetição" da textura
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vUV;

// Taper: x,z *= (1 + k*y). A seção transversal encolhe/cresce com a altura.
vec3 taper(vec3 p, float k) {
    float s = 1.0 + k * p.y;
    return vec3(p.x * s, p.y, p.z * s);
}

// Twist: gira (x,z) em torno de Y por um ângulo que cresce com a altura.
vec3 twist(vec3 p, float k) {
    float a = k * p.y;
    float c = cos(a), s = sin(a);
    return vec3(c * p.x - s * p.z, p.y, s * p.x + c * p.z);
}

// Bend: enrola o eixo Y num arco de círculo de raio R = 1/k, no plano XY.
// Um ponto na altura y vai para o ângulo theta = k*y desse arco.
vec3 bend(vec3 p, float k) {
    if (abs(k) < 1e-4) return p;  // curvatura ~0: identidade (evita 1/0)
    float theta = k * p.y;
    float c = cos(theta), s = sin(theta);
    float R = 1.0 / k;
    return vec3(c * (p.x - R) + R, -s * (p.x - R), p.z);
}

// Deformação completa, na ordem pedida.
vec3 deform(vec3 p) {
    return bend(twist(taper(p, uTaper), uTwist), uBend);
}

void main() {
    // ---- 1) Mistura A/B ----------------------------------------------------
    vec3 pos = mix(aPosA, aPosB, uT);
    vec3 n = mix(aNrmA, aNrmB, uT);
    // Normais opostas podem se anular no meio do caminho: evita normalize(0).
    n = (dot(n, n) > 1e-12) ? normalize(n) : vec3(0.0, 1.0, 0.0);

    // ---- 2) Deformação da posição -----------------------------------------
    vec3 dpos = deform(pos);

    // ---- 3) Normal da superfície DEFORMADA (cálculo numérico) -------------
    // A normal antiga não serve: a deformação estica e gira a superfície.
    // Ideia: a normal é perpendicular ao plano tangente. Pegamos dois
    // vetores t1, t2 no plano tangente ORIGINAL (perpendiculares a n),
    // andamos um passo pequeno eps em cada um e vemos para onde a
    // deformação leva esses pontos vizinhos. As diferenças
    //     d1 = deform(pos + eps*t1) - deform(pos)
    //     d2 = deform(pos + eps*t2) - deform(pos)
    // são (aproximadamente) os tangentes da superfície deformada, e
    // n' = normalize(cross(d1, d2)) é a nova normal. É uma derivada por
    // diferenças finitas: funciona para QUALQUER combinação de taper,
    // twist e bend, sem precisar derivar fórmula nenhuma, e roda na GPU.
    const float eps = 1e-3;
    // Escolhe um eixo auxiliar que não seja paralelo a n.
    vec3 helper = (abs(n.y) < 0.99) ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
    vec3 t1 = normalize(cross(helper, n));
    vec3 t2 = cross(n, t1);  // (t1, t2, n) forma uma base ortonormal destra
    vec3 d1 = deform(pos + eps * t1) - dpos;
    vec3 d2 = deform(pos + eps * t2) - dpos;
    vec3 c = cross(d1, d2);
    vec3 dn = (dot(c, c) > 1e-20) ? normalize(c) : n;
    // Orientação: a "normal esperada" é para onde a deformação leva um
    // passo na direção de n (o lado de fora). Se a nova normal apontar
    // para o lado oposto (deformação que espelha localmente, ex.: dobra
    // tão forte que passa do centro de curvatura), invertemos.
    vec3 expected = deform(pos + eps * n) - dpos;
    if (dot(dn, expected) < 0.0) dn = -dn;

    // ---- 4) Transformações de modelo, visão e projeção -------------------
    vec4 world = uModel * vec4(dpos, 1.0);
    vWorldPos = world.xyz;
    // uModel só tem rotação, então serve também para as normais.
    vNormal = mat3(uModel) * dn;
    gl_Position = uProj * uView * world;

    // Mapeamento paramétrico: a coordenada de textura é o (u,v) da grade.
    // Ela NÃO muda com a mistura nem com as deformações, então a textura
    // fica "colada" na superfície e estica/comprime junto com ela.
    vUV = aUV * uTexScale;
}
