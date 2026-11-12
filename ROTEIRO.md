# Morphing 3D: roteiro de apresentação

Projeto Final de Computação Gráfica (UDESC/CCT, prof. André Tavares da Silva)
Tema: **Alteração de geometrias: morphing de objetos tridimensionais**

---

## 1. Como compilar e executar

**Linux (Ubuntu/Debian)**

```bash
sudo apt install build-essential libglfw3-dev     # dependências
make                                              # ou: mkdir build && cd build && cmake .. && make
./morph3d
```

**Windows (MinGW)**: com o GLFW instalado no MinGW:

```bash
mingw32-make OS=Windows_NT
morph3d.exe
```

Rode a partir da pasta do projeto (o programa procura os shaders em `shaders/`).
Requer placa/driver com **OpenGL 3.3**.

### Controles

| Tecla | Ação |
|---|---|
| `ESPAÇO` | pausa / retoma a animação automática |
| `←` `→` (segurar) | controla o morph manualmente (diminui / aumenta *t*) |
| `1` `2` `3` `4` | escolhe a próxima forma (Esfera, Cubo, Toro, Cilindro) |
| `S` | alterna curva **linear** / **suavizada** (smoothstep) |
| `W` | alterna sólido / sólido + malha / só malha |
| `R` | liga/desliga a rotação do objeto |
| mouse (arrastar) | orbita a câmera |
| scroll | zoom |
| `ESC` | sair |

O **título da janela** mostra o par de formas, o valor de *t*, o modo, a curva e o FPS.

---

## 2. Sequência da demonstração (~5 min)

1. **Abrir o programa** e deixar rodar no automático: o objeto passa por
   esfera → cubo → toro → cilindro → esfera. *"Todas as formas têm exatamente
   o mesmo número de vértices e a mesma conectividade; só as posições mudam."*
2. **`W` duas vezes (só malha)**: mostrar que a malha é a mesma grade
   deformando-se. Voltar com `W` para sólido + malha.
3. **`ESPAÇO` (pausar)**, depois segurar **`→`** e **`←`**: mostrar *t* indo e
   voltando no título; o morph é contínuo e reversível.
4. **`S`**: comparar a curva linear (velocidade constante, começa e para
   "seco") com a smoothstep (acelera e desacelera suavemente).
5. **Teclas `1`–`4`**: escolher destinos fora da ordem (ex.: esfera → toro).
6. **Mouse/scroll**: orbitar e aproximar para mostrar o cubo com arestas
   limpas e a iluminação.
7. Abrir o código e mostrar **`morph.vert`** (onde a interpolação acontece) e
   **`shapes.cpp`** (como a correspondência é construída).

---

## 3. O que explicar (teoria)

### 3.1 O problema da correspondência
Morphing entre duas malhas exige saber **qual vértice de A vira qual vértice de B**.
A solução usada: todas as formas são **funções paramétricas da mesma grade (u, v)**:

- `u ∈ [0,1]` → ângulo θ = 2πu em torno do eixo Y
- `v ∈ [0,1]` → do topo (v = 0) até a base (v = 1)

Como a grade é a mesma (65 × 49 vértices), o vértice *i* da esfera corresponde
ao vértice *i* do cubo, do toro e do cilindro (**correspondência 1:1**). Os índices
dos triângulos (EBO) são **compartilhados** por todas as formas.

| Forma | Parametrização |
|---|---|
| Esfera | coordenadas esféricas: φ = πv |
| Cubo | tampa / lateral / tampa; contorno quadrado (direção projetada no quadrado) |
| Toro | θ no anel maior, φ = π − 2πv no tubo (equador da esfera → borda externa) |
| Cilindro | tampa / lateral / tampa; contorno circular |

### 3.2 Interpolação
Para cada vértice:

  **P(t) = (1 − t)·A + t·B**  e  **N(t) = normalize((1 − t)·N_A + t·N_B)**

A normal é interpolada e **renormalizada**, porque a média de dois vetores unitários
não é unitária. A cor também é interpolada.

### 3.3 Suavização temporal (easing)
O *t* linear é passado por **smoothstep: s(t) = 3t² − 2t³**, que tem derivada zero
em t = 0 e t = 1, então a forma "sai" e "chega" sem tranco.

### 3.4 Implementação em OpenGL moderno
- Cada forma fica em **um VBO** (posição + normal intercaladas), enviado **uma vez**.
- O **VAO** liga a forma de origem nos atributos `0/1` e a de destino nos `2/3`.
  Trocar de par = só chamar `glVertexAttribPointer` de novo, **sem reenviar dados**.
- A interpolação roda **na GPU** (vertex shader), com *t* como `uniform`; a CPU
  só atualiza um número por quadro.
- **Iluminação de Phong por pixel** no fragment shader (ambiente + difusa +
  especular), com iluminação dos dois lados.
- Câmera orbital com `lookAt` + `perspective` (matrizes implementadas em `mathutil.h`).
- `glPolygonMode` + `glPolygonOffset` para desenhar a malha sobre o sólido.

### 3.5 Tópicos da disciplina usados
Transformações geométricas (modelo, visão, projeção perspectiva), modelagem de
superfícies paramétricas, malhas poligonais e indexação, cálculo de normais,
modelo de iluminação de Phong, pipeline programável (shaders, VBO/VAO/EBO),
interação (teclado/mouse) e animação baseada em tempo (`dt`, independente de FPS).

---

## 4. Perguntas prováveis e respostas

**"Por que o cubo não fica serrilhado nas arestas?"**
Porque o cubo foi parametrizado com a mesma estrutura do cilindro: as bordas das
tampas caem exatamente na linha v = 0,25 e v = 0,75, e os cantos verticais em
θ = 45°, 135°… que são colunas da grade. Uma primeira versão projetava a esfera
no cubo e as arestas cortavam os triângulos, ficando serrilhadas. Há um
`static_assert` garantindo que a resolução da grade mantenha esse alinhamento.

**"Esfera e toro têm topologias diferentes (gênero 0 × gênero 1). Como dá certo?"**
A malha é uma grade aberta; a "costura" é feita por vértices com posições
coincidentes. Na esfera, a primeira e a última linha colapsam nos polos; no toro,
elas coincidem no anel interno. A conectividade não muda, só onde os vértices caem.

**"Por que não interpolar na CPU?"**
Daria certo, mas exigiria reenviar o VBO todo a cada quadro. Na GPU, cada
vértice é interpolado em paralelo e só um `uniform` muda.

**"Quais as limitações?"**
Interpolação **linear** pode causar encolhimento/auto-interseção no meio do
caminho, porque cada vértice anda em linha reta. Alternativas: interpolar em
coordenadas esféricas, usar morphing baseado em campos de distância (SDF) ou
correspondência automática entre malhas arbitrárias, que é bem mais complexa.

**"Por que renormalizar a normal?"**
`mix(N_A, N_B, t)` encurta o vetor (no caso extremo de normais opostas, vira zero),
e a iluminação de Phong supõe normal unitária.

---

## 5. Estrutura do código

```
morph3d/
├── src/
│   ├── main.cpp       janela, VAO/VBO, loop, animação, entrada, renderização
│   ├── shapes.cpp/.h  formas paramétricas na grade (u, v) e índices
│   ├── shader.cpp/.h  leitura/compilação/link dos shaders
│   └── mathutil.h     Vec3, Mat4, perspective, lookAt, rotateY
├── shaders/
│   ├── morph.vert     interpolação de posição e normal (o morphing em si)
│   └── morph.frag     iluminação de Phong
├── external/glad/     carregador OpenGL 3.3 core (gerado pelo glad2)
├── CMakeLists.txt
└── Makefile
```

---

## 6. Lembrete de entrega

O enunciado exige também a **lista da sequência de tarefas de cada membro**,
com data, hora de início e fim e tempo gasto (ex.: *"Fulano começou a resolver
um bug na função X às 23h do dia 22/01 até 5h do dia 23/01, levando
aproximadamente 4 horas"*). A falta dela causa perda de pontos.
