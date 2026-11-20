/* render.c - Desenho da cena (ver render.h). */
#include "render/render.h"

#include <glad/gl.h>
#include <stdlib.h>

#include "app/app.h"
#include "geometry/mathutil.h"
#include "geometry/morph.h"
#include "render/shader.h"

/* Objetos do OpenGL e posições dos uniforms (consultadas uma vez só). */
static GLuint program, vao, vbo, ebo;
static GLint loc_model, loc_view, loc_proj, loc_color, loc_light, loc_eye, loc_wire;

/* Memória (na CPU) para a forma intermediária calculada a cada quadro. */
static Vec3 *blend_pos, *blend_nrm;
static float *vertex_data; /* [px py pz nx ny nz] por vértice, formato do VBO */

int render_init(void) {
    char vert_path[512], frag_path[512];
    unsigned int *indices;

    /* ---- Shaders ---------------------------------------------------------- */
    shader_find("morph.vert", vert_path, sizeof(vert_path));
    shader_find("morph.frag", frag_path, sizeof(frag_path));
    program = shader_load_program(vert_path, frag_path);
    if (!program) return 0;
    loc_model = glGetUniformLocation(program, "uModel");
    loc_view = glGetUniformLocation(program, "uView");
    loc_proj = glGetUniformLocation(program, "uProj");
    loc_color = glGetUniformLocation(program, "uColor");
    loc_light = glGetUniformLocation(program, "uLightPos");
    loc_eye = glGetUniformLocation(program, "uEyePos");
    loc_wire = glGetUniformLocation(program, "uWireframe");

    /* ---- Memória para o morph -------------------------------------------- */
    blend_pos = (Vec3 *)malloc(VERTEX_COUNT * sizeof(Vec3));
    blend_nrm = (Vec3 *)malloc(VERTEX_COUNT * sizeof(Vec3));
    vertex_data = (float *)malloc(VERTEX_COUNT * 6 * sizeof(float));
    indices = (unsigned int *)malloc(INDEX_COUNT * sizeof(unsigned int));
    if (!blend_pos || !blend_nrm || !vertex_data || !indices) {
        free(indices);
        return 0;
    }

    /* ---- Buffers na GPU --------------------------------------------------- */
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    /* VBO: reescrito a cada quadro com a forma intermediária (DYNAMIC). */
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, VERTEX_COUNT * 6 * sizeof(float), NULL, GL_DYNAMIC_DRAW);
    /* atributo 0 = posição, atributo 1 = normal (intercalados no VBO) */
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    /* EBO: os triângulos da grade. São os mesmos para todas as formas
     * (todas usam a mesma grade), então é enviado uma única vez. */
    grid_build_indices(indices);
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, INDEX_COUNT * sizeof(unsigned int), indices,
                 GL_STATIC_DRAW);
    free(indices);

    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    return 1;
}

void render_shutdown(void) {
    if (ebo) glDeleteBuffers(1, &ebo);
    if (vbo) glDeleteBuffers(1, &vbo);
    if (vao) glDeleteVertexArrays(1, &vao);
    if (program) glDeleteProgram(program);
    free(blend_pos);
    free(blend_nrm);
    free(vertex_data);
}

/* Calcula o morph para o t atual e envia o resultado para o VBO. */
static void upload_morph(void) {
    const Shape *a = &app.shapes[app.shape_a];
    const Shape *b = &app.shapes[app.shape_b];
    int i;

    morph_blend(a, b, app.t, blend_pos, blend_nrm); /* <- o morph, em C */

    for (i = 0; i < VERTEX_COUNT; ++i) {
        float *d = &vertex_data[i * 6];
        d[0] = blend_pos[i].x; d[1] = blend_pos[i].y; d[2] = blend_pos[i].z;
        d[3] = blend_nrm[i].x; d[4] = blend_nrm[i].y; d[5] = blend_nrm[i].z;
    }
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, VERTEX_COUNT * 6 * sizeof(float), vertex_data);
}

void render_frame(int width, int height) {
    const Camera *c = &app.cam;
    Vec3 eye, color;
    Mat4 view, proj, model;

    glViewport(0, 0, width, height);
    glClearColor(0.08f, 0.09f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (width <= 0 || height <= 0) return; /* janela minimizada */

    upload_morph();

    /* Câmera orbital: posição em coordenadas esféricas em volta da origem. */
    eye = v3(c->dist * cosf(c->pitch) * sinf(c->yaw), c->dist * sinf(c->pitch),
             c->dist * cosf(c->pitch) * cosf(c->yaw));
    view = m4_lookat(eye, v3(0, 0, 0), v3(0, 1, 0));
    proj = m4_perspective(0.8f, (float)width / height, 0.1f, 100.0f);
    model = m4_rotate_y(app.obj_angle); /* rotação automática (tecla R) */
    color = morph_color(&app.shapes[app.shape_a], &app.shapes[app.shape_b], app.t);

    glUseProgram(program);
    glUniformMatrix4fv(loc_model, 1, GL_FALSE, model.m);
    glUniformMatrix4fv(loc_view, 1, GL_FALSE, view.m);
    glUniformMatrix4fv(loc_proj, 1, GL_FALSE, proj.m);
    glUniform3f(loc_color, color.x, color.y, color.z);
    glUniform3f(loc_light, 3.0f, 4.0f, 3.0f);
    glUniform3f(loc_eye, eye.x, eye.y, eye.z);

    /* Tecla W: sólido (triângulos preenchidos) ou só malha (arestas). */
    glUniform1i(loc_wire, app.wireframe);
    glPolygonMode(GL_FRONT_AND_BACK, app.wireframe ? GL_LINE : GL_FILL);

    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, INDEX_COUNT, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}
