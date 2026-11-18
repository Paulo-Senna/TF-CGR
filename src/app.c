/* app.c - Lógica do morph (animação, escolha de formas, curva de suavização). */
#include "app.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

App app;

/* Curva de suavização: começa e termina devagar (derivada zero nas pontas). */
static float smoothstep01(float x) { return x * x * (3.0f - 2.0f * x); }

/* Inversa de s(x) = 3x² - 2x³ em [0,1]. A cúbica tem solução fechada
 * trigonométrica: x = 1/2 - sen(asen(1 - 2y) / 3). */
static float inverse_smoothstep01(float y) {
    return 0.5f - sinf(asinf(clampf(1.0f - 2.0f * y, -1.0f, 1.0f)) / 3.0f);
}

void app_reset_camera(void) {
    app.cam.yaw = CAM_YAW;
    app.cam.pitch = CAM_PITCH;
    app.cam.dist = CAM_DIST;
    app.cam.dragging = 0;
}

void app_reset_deform(void) {
    app.deform.taper = 0.0f;
    app.deform.twist = 0.0f;
    app.deform.bend = 0.0f;
}

void app_reset_texture(void) {
    app.tex.current = TEX_NONE;
    app.tex.scale = 1.0f;
    app.tex.multiply = 1;
}

/* Copia uma string com truncamento seguro (strncpy não garante o '\0'). */
static void copy_str(char *dst, size_t size, const char *src) {
    snprintf(dst, size, "%s", src);
}

int app_load_image(const char *path) {
    TexState *tx = &app.tex;
    char err[sizeof(tx->message)];
    const char *base;
    int w = 0, h = 0;
    GLuint id = texture_load_file(path, &w, &h, err, sizeof(err));
    if (!id) {
        copy_str(tx->message, sizeof(tx->message), err);
        tx->message_is_error = 1;
        fprintf(stderr, "Textura: %s\n", err);
        return 0;
    }
    if (tx->ids[TEX_IMAGE]) glDeleteTextures(1, &tx->ids[TEX_IMAGE]);
    tx->ids[TEX_IMAGE] = id;
    tx->current = TEX_IMAGE;
    tx->multiply = 0; /* uma foto aparece com as cores originais */
    if (path != tx->path) copy_str(tx->path, sizeof(tx->path), path);

    /* Só o nome do arquivo, sem as pastas. */
    base = strrchr(path, '/');
    if (!base) base = strrchr(path, '\\');
    base = base ? base + 1 : path;
    snprintf(tx->image_info, sizeof(tx->image_info), "%.60s (%dx%d)", base, w, h);
    snprintf(tx->message, sizeof(tx->message), "Imagem carregada: %s", tx->image_info);
    tx->message_is_error = 0;
    printf("Textura: %s\n", tx->message);
    return 1;
}

void app_reset_state(void) {
    app.morph.from = SHAPE_SPHERE;
    app.morph.to = SHAPE_CUBE;
    app.morph.t = 0.0f;
    app.morph.hold = HOLD_TIME;
    app.morph.auto_play = 0; /* começa parado: o usuário liga no painel ou com ESPAÇO */
    app.morph.smooth = 1;
    app.morph.queued = -1;
    app_reset_deform();
    app_reset_texture();
    app_reset_camera();
    app.render_mode = RENDER_SOLID;
    app.rotate = 1;
}

float app_eased_t(void) { return app.morph.smooth ? smoothstep01(app.morph.t) : app.morph.t; }

/* Liga a forma A nos atributos 0/1 e a forma B nos atributos 2/3.
 * Não reenvia dados: só muda para onde os ponteiros de atributo apontam. */
void app_bind_shape_pair(void) {
    const GLsizei stride = 6 * sizeof(float);
    if (!app.vao) return;
    glBindVertexArray(app.vao);

    glBindBuffer(GL_ARRAY_BUFFER, app.vbos[app.morph.from]);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void *)0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void *)(3 * sizeof(float)));

    glBindBuffer(GL_ARRAY_BUFFER, app.vbos[app.morph.to]);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void *)0);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, (void *)(3 * sizeof(float)));

    glBindVertexArray(0);
}

void app_set_shapes(int from, int to) {
    app.morph.from = from;
    app.morph.to = to;
    app.morph.queued = -1; /* escolha explícita substitui o que estava na fila */
    app_bind_shape_pair();
}

void app_choose_target(int target) {
    MorphState *m = &app.morph;
    if (target < 0 || target >= SHAPE_COUNT || target == m->from) return;
    if (m->t == 0.0f) {
        /* Ainda não começou a deformar: troca o destino na hora. */
        m->to = target;
        m->hold = 0.0f;
        app_bind_shape_pair();
    } else {
        /* No meio de um morph: o destino escolhido vem em seguida. */
        m->queued = target;
    }
}

void app_set_t(float t) {
    app.morph.t = clampf(t, 0.0f, 1.0f);
    app.morph.auto_play = 0; /* mexer no slider assume o controle manual */
}

/* Trocar a curva no meio do morph mudaria a forma de repente, porque a
 * forma depende de eased(t), não de t. Para evitar o salto, convertemos t
 * de modo que eased(t) continue igual:
 *   suave -> linear: t' = s(t)       (o novo t linear já é o valor suavizado)
 *   linear -> suave: t' = s^-1(t)    (o t cujo valor suavizado é o atual)
 * A animação segue dali com a nova curva, sem descontinuidade. */
void app_set_smooth(int smooth) {
    MorphState *m = &app.morph;
    smooth = smooth ? 1 : 0;
    if (smooth == m->smooth) return;
    m->t = smooth ? inverse_smoothstep01(m->t) : smoothstep01(m->t);
    m->smooth = smooth;
}

/* Avança para o próximo par de formas quando um morph termina. */
static void finish_morph(void) {
    MorphState *m = &app.morph;
    m->from = m->to;
    if (m->queued >= 0 && m->queued != m->from) m->to = m->queued;
    else m->to = (m->to + 1) % SHAPE_COUNT;
    m->queued = -1;
    m->t = 0.0f;
    m->hold = HOLD_TIME;
    app_bind_shape_pair();
}

/* Atualiza o estado da animação (dt em segundos). */
void app_update(float dt, int manual_dir) {
    MorphState *m = &app.morph;

    if (m->auto_play) {
        if (m->hold > 0.0f) m->hold -= dt;
        else m->t += dt / MORPH_DURATION;
    }
    /* Controle manual com as setas (funciona com ou sem pausa). */
    m->t += (float)manual_dir * MANUAL_SPEED * dt;
    m->t = maxf(m->t, 0.0f);

    /* Chegou em B: passa ao próximo par. Só quando o tempo ou as setas
     * empurraram t; se foi o slider que levou t a 1, a forma fica parada em B. */
    if (m->t >= 1.0f && (m->auto_play || manual_dir != 0)) finish_morph();

    if (app.rotate) app.obj_angle += 0.4f * dt;
}
