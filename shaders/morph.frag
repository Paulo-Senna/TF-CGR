#version 330 core
// Fragment shader: iluminação de Phong (ambiente + difusa + especular)
// calculada por pixel.
in vec3 vWorldPos;
in vec3 vNormal;

uniform vec3 uColor;       // cor já interpolada entre as duas formas
uniform vec3 uLightPos;
uniform vec3 uEyePos;
uniform bool uWireframe;
uniform vec3 uWireColor;  // cor das linhas no modo malha

out vec4 fragColor;

void main() {
    if (uWireframe) {
        fragColor = vec4(uWireColor, 1.0);
        return;
    }
    vec3 N = normalize(vNormal);
    vec3 L = normalize(uLightPos - vWorldPos);
    vec3 V = normalize(uEyePos - vWorldPos);
    // Iluminação dos dois lados: durante o morphing podemos ver
    // o "lado de dentro" de alguma face; nesse caso inverte a normal.
    if (dot(N, V) < 0.0) N = -N;
    vec3 R = reflect(-L, N);

    float ambient  = 0.18;
    float diffuse  = max(dot(N, L), 0.0);
    float specular = pow(max(dot(R, V), 0.0), 48.0) * 0.6;

    vec3 color = uColor * (ambient + diffuse) + vec3(specular);
    fragColor = vec4(color, 1.0);
}
