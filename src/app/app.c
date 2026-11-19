/* app.c - Lógica do morph (animação, escolha de formas, formas adicionadas). */
#include "app/app.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "geometry/mesh_fit.h"
#include "io/obj.h"

App app;

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

void app_message(Message *m, int is_error, const char *text) {
    snprintf(m->text, sizeof(m->text), "%s", text);
    m->is_error = is_error;
    fprintf(is_error ? stderr : stdout, "%s\n", text);
}

/* Só o nome do arquivo, sem as pastas. */
static const char *base_name(const char *path) {
    const char *a = strrchr(path, '/'), *b = strrchr(path, '\\');
    if (b > a) a = b;
    return a ? a + 1 : path;
}

int app_load_image(const char *path) {
    TexState *tx = &app.tex;
    char text[sizeof(tx->msg.text)];
    int w = 0, h = 0;
    GLuint id = texture_load_file(path, &w, &h, text, sizeof(text));
    if (!id) {
        app_message(&tx->msg, 1, text);
        return 0;
    }
    if (tx->ids[TEX_IMAGE]) glDeleteTextures(1, &tx->ids[TEX_IMAGE]);
    tx->ids[TEX_IMAGE] = id;
    tx->current = TEX_IMAGE;
    tx->multiply = 0; /* uma foto aparece com as cores originais */
    snprintf(tx->path, sizeof(tx->path), "%s", path);
    snprintf(tx->image_info, sizeof(tx->image_info), "%.60s (%dx%d)", base_name(path), w, h);
    snprintf(text, sizeof(text), "Imagem carregada: %s", tx->image_info);
    app_message(&tx->msg, 0, text);
    return 1;
}

void app_upload_shape(int index) {
    const Shape *sh = &app.shapes[index];
    float *data = (float *)malloc(VERTEX_COUNT * 6 * sizeof(float));
    int i;
    if (!data) return;
    for (i = 0; i < VERTEX_COUNT; ++i) {
        float *d = &data[i * 6];
        d[0] = sh->positions[i].x; d[1] = sh->positions[i].y; d[2] = sh->positions[i].z;
        d[3] = sh->normals[i].x;   d[4] = sh->normals[i].y;   d[5] = sh->normals[i].z;
    }
    if (!app.vbos[index]) glGenBuffers(1, &app.vbos[index]);
    glBindBuffer(GL_ARRAY_BUFFER, app.vbos[index]);
    glBufferData(GL_ARRAY_BUFFER, VERTEX_COUNT * 6 * sizeof(float), data, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    free(data);
}

/* Cores das formas adicionadas (uma por vez, em ciclo). */
static Vec3 added_color(int n) {
    static const float colors[][3] = {
        {0.90f, 0.35f, 0.45f}, {0.25f, 0.75f, 0.80f}, {0.85f, 0.85f, 0.85f}, {0.95f, 0.65f, 0.35f}};
    const float *c = colors[n % 4];
    return v3(c[0], c[1], c[2]);
}

int app_add_shape_file(const char *path) {
    ObjMesh mesh;
    Shape *sh;
    char text[256], name[32];
    const char *base = base_name(path);
    size_t len;
    int index = app.shape_count, ok;

    if (index >= MAX_SHAPES) {
        app_message(&app.shape_msg, 1, "Limite de 9 formas atingido (teclas 1 a 9).");
        return 0;
    }
    if (!obj_read(path, &mesh, text, sizeof(text))) {
        app_message(&app.shape_msg, 1, text);
        return 0;
    }

    sh = &app.shapes[index];
    if (mesh.grid_file && mesh.vcount == VERTEX_COUNT) {
        /* Arquivo salvo por este programa: já está na grade, sem reamostrar. */
        ok = shape_from_grid(sh, mesh.positions, mesh.ncount == VERTEX_COUNT ? mesh.normals : NULL);
    } else {
        printf("Encaixando %s na grade (%d triangulos)...\n", base, mesh.tcount);
        ok = shape_fit_mesh(sh, mesh.positions, mesh.vcount, mesh.tris, mesh.tcount);
    }
    obj_free(&mesh);
    if (ok != 1) {
        app_message(&app.shape_msg, 1,
                    ok == 0 ? "Erro: memória insuficiente." : "Erro: o modelo está vazio.");
        return 0;
    }

    /* Nome = arquivo sem a extensão. */
    snprintf(name, sizeof(name), "%s", base);
    len = strlen(name);
    if (len > 4 && name[len - 4] == '.') name[len - 4] = '\0';
    snprintf(sh->name, sizeof(sh->name), "%s", name);
    sh->color = added_color(index - SHAPE_BUILTIN_COUNT);
    app_upload_shape(index);
    app.shape_count++;

    /* Mostra a forma nova na hora: ela vira a forma A, parada. */
    app_set_shapes(index, app.morph.from != index ? app.morph.from : SHAPE_SPHERE);
    app.morph.t = 0.0f;
    app.morph.hold = HOLD_TIME;
    app.morph.auto_play = 0;
    snprintf(text, sizeof(text), "Forma adicionada: %s (tecla %d)", sh->name, index + 1);
    app_message(&app.shape_msg, 0, text);
    return 1;
}

void app_reset_state(void) {
    app.morph.from = SHAPE_SPHERE;
    app.morph.to = SHAPE_CUBE;
    app.morph.t = 0.0f;
    app.morph.hold = HOLD_TIME;
    app.morph.auto_play = 0; /* começa parado: o usuário liga no painel ou com ESPAÇO */
    app.morph.queued = -1;
    app_reset_deform();
    app_reset_texture();
    app_reset_camera();
    app.render_mode = RENDER_SOLID;
    app.rotate = 1;
    app.shape_msg.text[0] = app.export_msg.text[0] = app.tex.msg.text[0] = '\0';
    app_bind_shape_pair(); /* não faz nada antes de o VAO existir */
}

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
    if (target < 0 || target >= app.shape_count || target == m->from) return;
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

/* Avança para o próximo par de formas quando um morph termina. */
static void finish_morph(void) {
    MorphState *m = &app.morph;
    m->from = m->to;
    if (m->queued >= 0 && m->queued != m->from) m->to = m->queued;
    else m->to = (m->to + 1) % app.shape_count;
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
