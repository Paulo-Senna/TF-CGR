/* deform.c - Versão em C das deformações do vertex shader (ver deform.h). */
#include "geometry/deform.h"

static Vec3 taper(Vec3 p, float k) {
    float s = 1.0f + k * p.y;
    return v3(p.x * s, p.y, p.z * s);
}

static Vec3 twist(Vec3 p, float k) {
    float a = k * p.y, c = cosf(a), s = sinf(a);
    return v3(c * p.x - s * p.z, p.y, s * p.x + c * p.z);
}

static Vec3 bend(Vec3 p, float k) {
    float theta, c, s, R;
    if (fabsf(k) < 1e-4f) return p; /* curvatura ~0: identidade */
    theta = k * p.y;
    c = cosf(theta);
    s = sinf(theta);
    R = 1.0f / k;
    return v3(c * (p.x - R) + R, -s * (p.x - R), p.z);
}

Vec3 deform_point(Vec3 p, const Deform *d) {
    return bend(twist(taper(p, d->taper), d->twist), d->bend);
}

Vec3 deform_normal(Vec3 p, Vec3 n, const Deform *d) {
    const float eps = 1e-3f;
    Vec3 dp = deform_point(p, d);
    Vec3 helper = fabsf(n.y) < 0.99f ? v3(0, 1, 0) : v3(1, 0, 0);
    Vec3 t1 = v3_normalize(v3_cross(helper, n));
    Vec3 t2 = v3_cross(n, t1);
    Vec3 d1 = v3_sub(deform_point(v3_add(p, v3_scale(t1, eps)), d), dp);
    Vec3 d2 = v3_sub(deform_point(v3_add(p, v3_scale(t2, eps)), d), dp);
    Vec3 c = v3_cross(d1, d2);
    Vec3 dn = v3_dot(c, c) > 1e-20f ? v3_normalize(c) : n;
    Vec3 expected = v3_sub(deform_point(v3_add(p, v3_scale(n, eps)), d), dp);
    if (v3_dot(dn, expected) < 0.0f) dn = v3_scale(dn, -1.0f);
    return dn;
}
