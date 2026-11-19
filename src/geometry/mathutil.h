/* mathutil.h - Vetores e matrizes 4x4 mínimos (sem dependências externas).
 * Matrizes em ordem column-major, como o OpenGL espera: m[coluna*4 + linha]. */
#ifndef MATHUTIL_H
#define MATHUTIL_H

#include <math.h>

#define PI_F 3.14159265358979f

typedef struct { float x, y, z; } Vec3;
typedef struct { float m[16]; } Mat4;

/* ---------------------------------------------------------------- escalares */
static inline float clampf(float x, float lo, float hi) { return x < lo ? lo : (x > hi ? hi : x); }
static inline float maxf(float a, float b) { return a > b ? a : b; }

/* ---------------------------------------------------------------- vetores */
static inline Vec3 v3(float x, float y, float z) { Vec3 r; r.x = x; r.y = y; r.z = z; return r; }
static inline Vec3 v3_add(Vec3 a, Vec3 b) { return v3(a.x + b.x, a.y + b.y, a.z + b.z); }
static inline Vec3 v3_sub(Vec3 a, Vec3 b) { return v3(a.x - b.x, a.y - b.y, a.z - b.z); }
static inline Vec3 v3_scale(Vec3 a, float s) { return v3(a.x * s, a.y * s, a.z * s); }
static inline float v3_dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

static inline Vec3 v3_cross(Vec3 a, Vec3 b) {
    return v3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

/* Normaliza; um vetor (quase) nulo vira (0,1,0) para nunca gerar NaN. */
static inline Vec3 v3_normalize(Vec3 v) {
    float len = sqrtf(v3_dot(v, v));
    return len > 1e-8f ? v3_scale(v, 1.0f / len) : v3(0.0f, 1.0f, 0.0f);
}

/* Interpolação linear: (1 - t)*a + t*b */
static inline Vec3 v3_lerp(Vec3 a, Vec3 b, float t) {
    return v3_add(v3_scale(a, 1.0f - t), v3_scale(b, t));
}

/* ---------------------------------------------------------------- matrizes */
static inline Mat4 m4_identity(void) {
    Mat4 r;
    int i;
    for (i = 0; i < 16; ++i) r.m[i] = 0.0f;
    r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
    return r;
}

/* r = a * b */
static inline Mat4 m4_mul(Mat4 a, Mat4 b) {
    Mat4 r;
    int c, l, k;
    for (c = 0; c < 4; ++c)
        for (l = 0; l < 4; ++l) {
            float s = 0.0f;
            for (k = 0; k < 4; ++k) s += a.m[k * 4 + l] * b.m[c * 4 + k];
            r.m[c * 4 + l] = s;
        }
    return r;
}

/* Projeção perspectiva (equivalente ao gluPerspective). */
static inline Mat4 m4_perspective(float fovy_rad, float aspect, float z_near, float z_far) {
    Mat4 r = m4_identity();
    float f = 1.0f / tanf(fovy_rad / 2.0f);
    r.m[0] = f / aspect;
    r.m[5] = f;
    r.m[10] = (z_far + z_near) / (z_near - z_far);
    r.m[11] = -1.0f;
    r.m[14] = (2.0f * z_far * z_near) / (z_near - z_far);
    r.m[15] = 0.0f;
    return r;
}

/* Matriz de visão (equivalente ao gluLookAt). */
static inline Mat4 m4_lookat(Vec3 eye, Vec3 center, Vec3 up) {
    Vec3 f = v3_normalize(v3_sub(center, eye));
    Vec3 s = v3_normalize(v3_cross(f, up));
    Vec3 u = v3_cross(s, f);
    Mat4 r = m4_identity();
    r.m[0] = s.x;  r.m[4] = s.y;  r.m[8] = s.z;
    r.m[1] = u.x;  r.m[5] = u.y;  r.m[9] = u.z;
    r.m[2] = -f.x; r.m[6] = -f.y; r.m[10] = -f.z;
    r.m[12] = -v3_dot(s, eye);
    r.m[13] = -v3_dot(u, eye);
    r.m[14] = v3_dot(f, eye);
    return r;
}

/* Rotação de 'a' radianos em torno do eixo Y. */
static inline Mat4 m4_rotate_y(float a) {
    Mat4 r = m4_identity();
    r.m[0] = cosf(a);  r.m[8] = sinf(a);
    r.m[2] = -sinf(a); r.m[10] = cosf(a);
    return r;
}

#endif /* MATHUTIL_H */
