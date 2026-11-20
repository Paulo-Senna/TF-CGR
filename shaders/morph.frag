#version 330 core
// Fragment shader: cor de cada pixel com iluminação de Phong
// (ambiente + difusa + especular).
in vec3 vWorldPos;
in vec3 vNormal;

uniform vec3 uColor;      // cor da forma (também interpolada entre A e B)
uniform vec3 uLightPos;   // posição da luz
uniform vec3 uEyePos;     // posição da câmera
uniform bool uWireframe;  // modo "só malha" (tecla W): linhas sem iluminação

out vec4 fragColor;

void main() {
    if (uWireframe) {
        fragColor = vec4(uColor, 1.0);
        return;
    }

    vec3 N = normalize(vNormal);
    vec3 L = normalize(uLightPos - vWorldPos);  // direção até a luz
    vec3 V = normalize(uEyePos - vWorldPos);    // direção até a câmera
    // Durante o morph (e no plano) dá para ver o "lado de dentro" de
    // alguma face; nesse caso invertemos a normal para ela ser iluminada.
    if (dot(N, V) < 0.0) N = -N;
    vec3 R = reflect(-L, N);                    // reflexão da luz

    float ambient  = 0.18;                                // luz de fundo
    float diffuse  = max(dot(N, L), 0.0);                 // Lambert
    float specular = pow(max(dot(R, V), 0.0), 48.0) * 0.6; // brilho

    fragColor = vec4(uColor * (ambient + diffuse) + vec3(specular), 1.0);
}
