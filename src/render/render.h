/* render.h - Desenho da cena com OpenGL 3.3.
 *
 * A cada quadro:
 *   1. o morph é calculado em C (geometry/morph.c) para o t atual;
 *   2. o resultado (posições + normais) é enviado para um único VBO;
 *   3. os shaders só aplicam câmera e iluminação e desenham os triângulos.
 * Ou seja, a GPU apenas DESENHA a forma; quem a transforma é o nosso código. */
#ifndef RENDER_H
#define RENDER_H

/* Carrega os shaders e cria os buffers. Retorna 0 em caso de erro. */
int render_init(void);
void render_shutdown(void);

/* Desenha a forma atual (estado em 'app') numa janela de width x height pixels. */
void render_frame(int width, int height);

#endif /* RENDER_H */
