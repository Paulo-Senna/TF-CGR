# TF-CGR

Projeto Final de Computação Gráfica (UDESC/CCT): **alteração de geometrias**.

Um programa em **C99 + OpenGL 3.3** que faz o **morph** (metamorfose) de um
objeto 3D em outro. Você escolhe a forma inicial (A) e a final (B) e, segurando
as setas, vê A se transformar em B. Dá para parar em qualquer ponto do caminho
(30% de A e 70% de B, por exemplo) e exportar essa forma intermediária como `.obj`.

O morph é **código nosso, em C**: não usamos nenhuma biblioteca de morph,
de geometria ou de interface. As únicas bibliotecas externas são:

- [GLFW](https://www.glfw.org/): abre a janela e lê teclado e mouse (instalada no sistema);
- [glad](https://github.com/Dav1dde/glad): carrega as funções do OpenGL 3.3 (em `external/`).

## Como o morph funciona

Todo morph 3D precisa resolver dois problemas (Alexa, 2002):

1. **Correspondência:** qual ponto de A vira qual ponto de B?
2. **Interpolação:** por qual caminho cada ponto vai de A até o seu par em B?

**1. Correspondência por uma grade comum** (`src/geometry/shapes.c`).
Todas as formas são construídas sobre a **mesma grade (u, v)** de 65 × 49 vértices:
`u` dá a volta em torno do eixo Y e `v` vai do topo até a base. A esfera, o cubo,
o toro, o cilindro e o plano têm, cada um, uma função que recebe `(u, v)` e devolve
um ponto da superfície. Por isso o vértice *i* da esfera corresponde ao vértice
*i* do cubo, e as duas malhas têm o mesmo número de vértices e os mesmos triângulos.

**Modelos importados** (`src/geometry/mesh_fit.c`). Um `.obj` qualquer tem uma
malha própria, então ele é **reamostrado** na nossa grade. Para cada vértice
`(u, v)` da esfera, um raio sai do centro do modelo naquela direção e o ponto
onde ele fura a superfície vira o vértice `(u, v)` do modelo. É a mesma ideia
da projeção na esfera de Kent, Carlson e Parent (1992), que funciona para
objetos "estrelados" (todo ponto da superfície é visível a partir do centro).

**2. Interpolação linear** (`src/geometry/morph.c`). Com a correspondência
pronta, a forma intermediária é calculada a cada quadro, vértice a vértice:

```
P(t) = (1 - t) * A + t * B        t = 0 -> forma A,  t = 1 -> forma B
```

As normais (usadas na iluminação) e a cor também são interpoladas. Segurar
`→` aumenta `t` e segurar `←` diminui; soltando a tecla, o morph para onde está.
O resultado vai para a GPU (`src/render/render.c`), que só desenha a forma com
iluminação de Phong (`shaders/`).

## Comandos

Todos pelo teclado. A lista aparece no terminal ao abrir (e com `H`), junto com
uma barra mostrando quanto da forma já é A e quanto já é B:

```
  Esfera [############--------] Cubo   60%
```

| Tecla | Ação |
|---|---|
| `←` / `→` (segurar) | transforma A em B (e volta) |
| `Q`, depois `1`–`9`, depois `1`–`9` | escolhe a forma A e depois a B (B precisa ser diferente de A) |
| `,` / `.` (segurar) | zoom: aproxima / afasta |
| `W` | alterna sólido / só malha |
| `R` | liga / desliga a rotação do objeto |
| Mouse (botão esquerdo) | arrastar gira a câmera |
| `I` | importa uma forma `.obj` (ou arraste o arquivo para a janela) |
| `O` | exporta a forma da tela como `.obj` (+ `.mtl` com a cor) |
| `H` | mostra a ajuda de novo |
| `ESC` | sai |

Formas prontas: `1` Esfera, `2` Cubo, `3` Toro, `4` Cilindro, `5` Plano.
As importadas ganham as teclas seguintes (até `9`).

## Compilar e rodar

Linux (Ubuntu/Debian):

```bash
sudo apt install build-essential cmake libglfw3-dev
make                      # ou: cmake -S . -B build && cmake --build build
./morph3d                 # rode a partir da pasta do projeto
```

Windows (MinGW, com GLFW instalado): `mingw32-make OS=Windows_NT`

As janelas de abrir/salvar arquivo usam o diálogo nativo no Windows e o
`zenity` (GNOME) ou `kdialog` (KDE) no Linux.

Opções de linha de comando (todas opcionais):

```bash
./morph3d --forma modelo.obj               # já abre com um modelo seu importado
./morph3d --mistura 0.5 --exportar meio.obj  # exporta a forma 50% Esfera / 50% Cubo
```

## Organização do código

```
src/
  main.c              janela, teclado/mouse e laço principal
  app/app.c           estado do programa e o que cada tecla faz (mensagens no terminal)
  geometry/
    mathutil.h        vetores e matrizes 4x4
    shapes.c          as 5 formas, todas na mesma grade (u, v)   <- correspondência
    mesh_fit.c        coloca um .obj na grade por lançamento de raios
    morph.c           P(t) = (1 - t) * A + t * B                  <- o morph
  render/
    shader.c          lê e compila os shaders
    render.c          envia a forma para a GPU e desenha
  io/
    obj.c             lê e escreve arquivos .obj
    export.c          exporta a forma da tela
    file_dialog.c     janela do sistema para escolher arquivo
shaders/              vertex e fragment shader (câmera e iluminação de Phong)
```

## Referências

1. KENT, J. R.; CARLSON, W. E.; PARENT, R. E. **Shape transformation for polyhedral objects.**
   *ACM SIGGRAPH Computer Graphics*, v. 26, n. 2 (SIGGRAPH '92), p. 47–54, 1992.
   DOI: [10.1145/142920.134007](https://doi.org/10.1145/142920.134007).
   Morph entre poliedros: primeiro se cria uma correspondência entre as malhas
   (para objetos "estrelados", projetando-as na esfera a partir do centro) e depois
   os vértices correspondentes são interpolados. É a base da nossa grade comum e
   do encaixe de modelos `.obj`.

2. LAZARUS, F.; VERROUST, A. **Three-dimensional metamorphosis: a survey.**
   *The Visual Computer*, v. 14, p. 373–389, 1998.
   DOI: [10.1007/s003710050149](https://doi.org/10.1007/s003710050149).
   Revisão das técnicas de morph 3D, organizadas pela representação do objeto
   (malhas, volumes, implícitas...). Situa a nossa abordagem: morph de **malhas**
   com correspondência por parametrização.

3. ALEXA, M. **Recent advances in mesh morphing.**
   *Computer Graphics Forum*, v. 21, n. 2, p. 173–198, 2002.
   DOI: [10.1111/1467-8659.00575](https://doi.org/10.1111/1467-8659.00575).
   Divide o morph de malhas em **correspondência** (incluindo parametrização num
   domínio comum, como a esfera) e **interpolação** (a linear é a mais simples).
   É a divisão usada na explicação acima e em `src/geometry/morph.h`.
