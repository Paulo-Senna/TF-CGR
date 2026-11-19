# Laboratório de Deformação: roteiro de apresentação

Projeto Final de Computação Gráfica (UDESC/CCT, prof. André Tavares da Silva)
Grupo: Paulo e Leonardo
Tema: **Alteração de geometrias: deformação e morphing de objetos tridimensionais**

O programa é um "laboratório" onde o usuário escolhe duas formas, **mistura**
uma na outra (morphing), aplica **deformações** (taper, twist, bend) e usa
**texturas** para enxergar como a superfície estica e comprime. Tudo é
controlado por um painel clicável (e pelos atalhos de teclado).

---

## 1. Como compilar e executar

**Linux (Ubuntu/Debian)**

```bash
sudo apt install build-essential cmake libglfw3-dev   # dependências
make                                                  # ou: cmake -S . -B build && cmake --build build
./morph3d
```

**Windows (MinGW)**: com o GLFW instalado no MinGW:

```bash
mingw32-make OS=Windows_NT
morph3d.exe
```

- Requer placa/driver com **OpenGL 3.3**.
- Com `make`, rode a partir da pasta do projeto (os shaders ficam em `shaders/`).
  O executável do CMake (`build/morph3d`) acha os shaders de qualquer pasta.
- `make DEBUG=1` gera uma build de depuração com *AddressSanitizer* e *UBSan*
  (rode `make clean` antes de trocar entre a build normal e a de depuração).
- As janelas de escolher arquivo usam o diálogo nativo no Windows e o `zenity`
  (ou `kdialog`) no Linux. Nada precisa ser reiniciado: formas e texturas
  adicionadas entram na hora.

**Opções de linha de comando** (todas opcionais):

| Opção | Efeito |
|---|---|
| `./morph3d foto.png` | já abre usando a imagem como textura |
| `--forma modelo.obj` | já abre com o modelo adicionado como forma |
| `--exportar saida.obj` | exporta a forma exibida ao abrir (útil para testes) |
| `--captura arq.ppm` | salva um quadro em PPM e fecha (passo de tempo fixo, reprodutível) |
| `--quadros N` | qual quadro capturar (padrão 100) |

---

## 2. Controles

### Teclado e mouse (os mesmos da versão anterior)

| Tecla | Ação |
|---|---|
| `ESPAÇO` | liga / pausa a animação automática |
| `←` `→` (segurar) | controla o morph manualmente (diminui / aumenta *t*) |
| `1` a `5` | escolhe a próxima forma: 1 Esfera, 2 Cubo, 3 Toro, 4 Cilindro, 5 Plano (as formas adicionadas ganham `6` a `9`; o teclado numérico também funciona) |
| `W` | alterna sólido / sólido + malha / só malha |
| `R` | liga/desliga a rotação do objeto |
| mouse (arrastar na cena) | orbita a câmera |
| scroll (na cena) | zoom |
| arrastar um PNG/JPG para a janela | usa a imagem como textura |
| arrastar um .obj para a janela | adiciona o modelo como forma |
| `ESC` | sair |

Com o mouse **sobre o painel**, arrastar e rolar mexem no painel, não na câmera.

O **título da janela** mostra o par de formas, o destino na fila
(ex.: `(próximo: Toro)`), o valor de *t*, o modo e o FPS.

### Painel (à esquerda; o "–" no título recolhe o painel)

| Seção | Controles |
|---|---|
| **Forma e mistura** | Forma A, Forma B, slider *t*, % exibida, animação automática, *Adicionar forma (.obj)...* |
| **Deformações (Barr)** | sliders Taper [-0,9; 0,9], Twist [-6; 6] rad/unid., Bend [-1,5; 1,5] e *Zerar deformações* |
| **Textura** | Nenhuma / Xadrez / Listras / Grade UV / Imagem carregada, *Repetição*, *Multiplicar pela cor da forma*, *Adicionar imagem (PNG/JPG)...* |
| **Visualização** | modo de desenho, distância (zoom), rotação, *Resetar câmera* |
| **Atalhos de teclado** | lista resumida das teclas |
| (fim do painel) | *Resetar tudo* e *Exportar 3D (.obj)...* |

Ao abrir, só **Forma e mistura** e **Atalhos de teclado** vêm expandidas; as
outras seções abrem com um clique no título.

**Adicionar forma**: abre a janela de arquivos para escolher um modelo `.obj`.
O modelo é encaixado na grade (seção 4.8), vira a *Forma A* (parado, *t* = 0) e
entra na lista das formas e nas teclas `6` a `9` (até 4 formas adicionadas).

**Exportar 3D**: abre a janela de salvar; na pasta escolhida são criados
`nome.obj` (malha com a mistura e as deformações atuais), `nome.mtl` (cor) e,
se houver textura, `nome_textura.tga` (xadrez/listras/grade) ou uma cópia da
imagem carregada. Abre no Blender, MeshLab etc., e também pode ser reaberto aqui
com *Adicionar forma* (sem perda: o arquivo já está na ordem da grade).

O programa abre com a animação automática **desligada** (forma parada na esfera);
ligue pelo checkbox *Animação automática* ou com `ESPAÇO`. *Resetar tudo* também
volta com ela desligada.

Mexer no slider *t* desliga a animação automática (o usuário assume o controle).
Se o slider chega em *t* = 1, a forma fica parada em B; a seta `→` ou a animação
automática passam para o próximo par.

---

## 3. Sequência da demonstração (~6 min)

1. **Abrir o programa** (≈30 s). Ele abre **parado** na esfera (animação
   automática desligada). Marcar *Animação automática* no painel (ou `ESPAÇO`):
   o morph passa por esfera → cubo → toro → cilindro → plano → esfera. *"Todas as formas têm
   exatamente o mesmo número de vértices e a mesma conectividade; só as
   posições mudam."*
2. **`W` duas vezes (só malha)** (≈30 s): a malha é a mesma grade se
   deformando. Voltar com `W` para sólido.
3. **Mistura pelo painel** (≈1 min): escolher *Forma A = Cubo*, *Forma B = Toro*
   e arrastar o slider *t* devagar. Teclas `1`–`5`
   durante um morph: o destino aparece no título como "próximo".
4. **Esfera torcida** (≈45 s): *Resetar tudo*, abrir *Textura* e escolher
   *Xadrez*; abrir *Deformações* e subir o *Twist*. *"A esfera é simétrica em
   torno de Y, então a torção não muda a silhueta... mas o xadrez mostra que
   cada camada girou."* Mostrar a malha com `W`.
5. **Cilindro dobrado** (≈45 s): Forma A = Cilindro, textura *Grade UV*, *Bend*
   em ~1: as linhas mostram o lado de fora esticado e o de dentro comprimido.
   Combinar com *Twist*.
6. **Cone por taper** (≈30 s): Cilindro com *Taper* em -0,8 e xadrez: vira
   tronco de cone; os quadrados ficam menores no topo.
7. **Plano → Esfera** (≈30 s): Forma A = Plano, Forma B = Esfera, textura
   *Grade UV*, arrastar *t*: o plano se fecha numa esfera.
8. **Imagem e modelo próprios** (≈1 min): *Adicionar imagem* (janela de
   arquivos) e aplicar deformações em cima. *Adicionar forma* com um `.obj` e
   fazer o morph Esfera → modelo.
9. **Exportar** (≈30 s): *Exportar 3D*, salvar e mostrar o `.obj` aberto num
   visualizador (ou reabrir com *Adicionar forma*).
10. **Código** (≈1 min): abrir `shaders/morph.vert` (mistura, `deform()` e o
    cálculo da normal) e `src/geometry/shapes.c` (a grade comum).

---

## 4. O que explicar (teoria)

### 4.1 O problema da correspondência

Morphing entre duas malhas exige saber **qual vértice de A vira qual vértice de B**.
A solução usada: todas as formas são **funções paramétricas da mesma grade (u, v)**:

- `u ∈ [0,1]` → ângulo θ = 2πu em torno do eixo Y
- `v ∈ [0,1]` → do topo (v = 0) até a base (v = 1)

Como a grade é a mesma (65 × 49 = 3185 vértices), o vértice *i* de uma forma
corresponde ao vértice *i* de todas as outras (**correspondência 1:1**). Os índices
dos triângulos (EBO) são **compartilhados**.

| Forma | Parametrização |
|---|---|
| Esfera | coordenadas esféricas: φ = πv |
| Cubo | tampa / lateral / tampa; contorno quadrado (direção projetada no quadrado) |
| Toro | θ no anel maior, φ = π − 2πv no tubo (equador da esfera → borda externa) |
| Cilindro | tampa / lateral / tampa; contorno circular |
| Plano | x = (u − 0,5)·2, z = (v − 0,5)·2, y = 0, normal (0, 1, 0) |

### 4.2 Interpolação (mistura)

Para cada vértice, no vertex shader:

  **P(t) = (1 − t)·A + t·B**  e  **N(t) = normalize((1 − t)·N_A + t·N_B)**

A normal é interpolada e **renormalizada**, porque a média de dois vetores unitários
não é unitária (se as normais forem opostas, a soma é zero; nesse caso o shader
usa uma normal padrão em vez de dividir por zero). A cor também é interpolada.

### 4.3 Deformações de Barr (o principal do trabalho)

Aplicadas **depois** da mistura, em espaço do objeto, ao longo do eixo Y, na ordem
**taper → twist → bend** (função `deform()` em `morph.vert`):

| Deformação | Parâmetro | Fórmula |
|---|---|---|
| **Taper** (afunilamento) | k | x' = x·(1 + k·y), z' = z·(1 + k·y), y' = y |
| **Twist** (torção) | k (rad por unidade) | α = k·y; x' = x·cos α − z·sen α, z' = x·sen α + z·cos α |
| **Bend** (dobra) | k (curvatura = 1/R) | θ = k·y, R = 1/k; x' = cos θ·(x − R) + R, y' = −sen θ·(x − R), z' = z |

O bend "enrola" o eixo Y num arco de círculo de raio R: um ponto na altura *y*
vai para o ângulo θ = k·y desse arco. Para |k| < 10⁻⁴ é usada a identidade
(evita dividir por zero; a dobra tende à identidade quando k → 0).

A **ordem importa**: deformações não comutam. Por exemplo, twist depois de bend
giraria em torno do eixo Y original, não do eixo já curvado.

### 4.4 Normais depois da deformação (cálculo numérico)

A normal antiga não serve: a deformação estica e gira a superfície. Em vez de
derivar a fórmula da normal de cada combinação de deformações (a normal correta é
**J⁻ᵀ·N**, com J a jacobiana da deformação), o shader calcula por **diferenças finitas**:

1. monta dois vetores **t₁, t₂** perpendiculares à normal interpolada **n**
   (base ortonormal destra t₁, t₂, n);
2. com ε = 10⁻³, calcula para onde a deformação leva os vizinhos:
   **d₁ = deform(p + ε t₁) − deform(p)**, **d₂ = deform(p + ε t₂) − deform(p)**;
3. **n' = normalize(d₁ × d₂)**: os dois vetores são tangentes à superfície
   deformada, então o produto vetorial é a nova normal;
4. orientação: a "normal esperada" é **deform(p + ε n) − deform(p)** (para onde vai
   um passo para fora); se `dot(n', esperada) < 0`, inverte. Isso só acontece onde a
   deformação espelha localmente (ex.: dobra que passa do centro de curvatura).

Vantagens: funciona para **qualquer** combinação de deformações, roda na GPU e
custa só 3 avaliações extras de `deform()` por vértice. Sem deformação,
deform é a identidade e n' = t₁ × t₂ = n (o resultado antigo).

### 4.5 Mapeamento de textura paramétrico

A coordenada de textura de cada vértice é o próprio **(u, v)** da grade, igual
para todas as formas (um único VBO no atributo 4). Ela **não muda** com a
mistura nem com as deformações, então a textura fica "colada" na superfície:

- **Xadrez**: quadrados maiores = superfície esticada; menores = comprimida;
- **Listras** (em u): com twist viram hélices;
- **Grade UV** (vermelho cresce com u, verde com v): cada ponto tem cor única,
  dá para ver para onde cada região da malha foi.

As texturas procedurais são geradas na CPU (256 × 256 RGBA), com `GL_REPEAT`
(o slider *Repetição* multiplica o UV), **mipmaps** e filtro trilinear. Imagens
PNG/JPG são lidas com **stb_image**; a imagem é espelhada na horizontal porque *u*
cresce da direita para a esquerda para quem olha a forma de fora (sem isso,
textos apareceriam invertidos). No fragment shader: cor base = textura, ou
textura × cor da forma, ou só a cor; depois vem o Phong.

### 4.6 Implementação em OpenGL moderno

- Cada forma fica em **um VBO** (posição + normal intercaladas), enviado **uma vez**.
- O **VAO** liga a forma A nos atributos `0/1`, a B nos `2/3` e os UVs no `4`.
  Trocar de par = só chamar `glVertexAttribPointer` de novo, **sem reenviar dados**.
- Mistura e deformações rodam **na GPU** (vertex shader); a CPU só atualiza
  alguns `uniform`s por quadro (posições consultadas uma vez, no início).
- **Iluminação de Phong por pixel** (ambiente + difusa + especular), dos dois lados.
- Câmera orbital com `lookAt` + `perspective` (implementadas em `mathutil.h`).
- `glPolygonMode` + `glPolygonOffset` para desenhar a malha sobre o sólido.
- **Interface** com Nuklear (modo imediato): o painel é descrito de novo a cada
  quadro, lendo e escrevendo direto no estado do programa.

### 4.7 Tópicos da disciplina usados

Transformações geométricas (modelo, visão, projeção perspectiva), **deformações
não lineares** (Barr), modelagem de superfícies paramétricas, malhas poligonais e
indexação, cálculo de normais (analítico e numérico), modelo de iluminação de Phong,
**mapeamento de textura** (coordenadas, repetição, filtragem, mipmaps), pipeline
programável (shaders, VBO/VAO/EBO), interação (teclado, mouse, interface gráfica)
e animação baseada em tempo (`dt`, independente de FPS).

### 4.8 Modelos importados: encaixe na grade e exportação

Um `.obj` qualquer tem a sua própria malha (outro número de vértices, outra
ordem), então não existe correspondência 1:1 com as nossas formas. A solução
(`src/geometry/mesh_fit.c`) é **reamostrar** o modelo na grade:

1. o modelo é centralizado (centro da caixa envolvente) e escalado para caber
   na esfera de raio 1;
2. para cada vértice (u, v) da grade, sai um **raio do centro** na direção do
   vértice correspondente da esfera; o ponto mais externo em que o raio fura a
   superfície (interseção raio-triângulo de Möller-Trumbore) vira a posição;
3. as normais são calculadas pela própria grade (produto vetorial dos vizinhos).

Assim o morph Esfera → modelo é natural: cada ponto da esfera "anda" em linha
reta até o modelo. Para não testar todos os triângulos em cada raio, os
triângulos são separados por faixa de latitude (só os da faixa do raio são
testados). Limites: partes que o centro não enxerga (cavidades, a alça de uma
xícara) se perdem, e arestas vivas ficam um pouco serrilhadas (a grade tem
65×49 vértices).

A **exportação** (`src/io/export.c`) refaz na CPU a mesma conta do vertex
shader (mistura + `deform()` + normal numérica, em `src/geometry/deform.c`) e
grava `.obj` + `.mtl`. O arquivo leva o comentário `# morph3d-grade`: ao ser
reaberto, os vértices já estão na ordem da grade e entram direto, sem reamostrar.

---

## 5. Perguntas prováveis e respostas

**"Por que o cubo não fica serrilhado nas arestas?"**
O cubo foi parametrizado com a mesma estrutura do cilindro: as bordas das tampas
caem exatamente em v = 0,25 e v = 0,75, e os cantos verticais em θ = 45°, 135°…,
que são colunas da grade. Uma verificação em tempo de compilação (`shapes.h`)
garante que a resolução da grade mantenha esse alinhamento.

**"Esfera e toro têm topologias diferentes (gênero 0 × gênero 1). Como dá certo?"**
A malha é uma grade aberta; a "costura" é feita por vértices com posições
coincidentes. Na esfera, a primeira e a última linha colapsam nos polos; no toro,
elas coincidem no anel interno. A conectividade não muda, só onde os vértices caem.

**"Por que calcular a normal numericamente e não pela fórmula?"**
A fórmula exata é J⁻ᵀN, e a jacobiana de taper∘twist∘bend é trabalhosa e teria
que ser refeita a cada deformação nova. As diferenças finitas usam a própria
`deform()`: qualquer deformação nova já sai com normais certas. O erro com
ε = 10⁻³ é invisível na iluminação.

**"Por que a esfera torcida parece igual sem textura?"**
Porque a torção gira cada camada horizontal em torno de Y, e a esfera é simétrica
em torno de Y: cada camada cai sobre ela mesma. Só a textura (ou a malha com `W`)
mostra que os pontos se moveram. É um bom exemplo de por que a textura ajuda a
visualizar deformações.

**"Por que a ordem é taper → twist → bend?"**
Taper e twist são definidos em relação ao eixo Y reto; aplicando-os primeiro,
eles atuam sobre o objeto ainda reto, e o bend curva o resultado já afunilado
e torcido. Se o bend viesse antes, o eixo já estaria curvado e o resultado
seria outro (as deformações não comutam).

**"Por que não interpolar/deformar na CPU?"**
Daria certo, mas exigiria reenviar o VBO a cada quadro. Na GPU cada vértice é
processado em paralelo e só alguns `uniform`s mudam.

**"Quais as limitações?"**
A interpolação é **linear**: cada vértice anda em linha reta, o que pode causar
encolhimento ou auto-interseção no meio do caminho (bem visível em Plano → Esfera).
Alternativas: interpolar em coordenadas esféricas, morphing por campos de
distância (SDF) ou correspondência automática entre malhas arbitrárias. As
deformações também podem gerar auto-interseção com valores extremos (ex.:
bend forte no plano, que passa do centro de curvatura).

**"Por que renormalizar a normal?"**
`mix(N_A, N_B, t)` encurta o vetor (no caso extremo de normais opostas, vira zero),
e a iluminação de Phong supõe normal unitária.

**"Por que reescrever em C?"**
A disciplina foi toda em C. O código usa structs + funções (`v3_add`, `m4_mul`...),
arrays com `malloc/free` e compila em C99 com `-Wall -Wextra` sem avisos.

---

## 6. Estrutura do código

```
TF-CGR/
├── src/
│   ├── main.c                janela, laço principal, entrada, desenho da cena
│   ├── app/
│   │   └── app.c / .h        estado do programa e lógica do morph (fila, slider,
│   │                         adicionar forma)
│   ├── geometry/
│   │   ├── mathutil.h        Vec3, Mat4, perspective, lookAt, rotate_y
│   │   ├── shapes.c / .h     formas paramétricas na grade (u, v), índices e UVs
│   │   ├── deform.c / .h     taper/twist/bend na CPU (cópia do shader, para exportar)
│   │   └── mesh_fit.c / .h   encaixa um modelo .obj qualquer na grade
│   ├── render/
│   │   ├── shader.c / .h     leitura/compilação/link dos shaders
│   │   └── texture.c / .h    texturas procedurais e carregamento PNG/JPG
│   ├── io/
│   │   ├── obj.c / .h        leitura e escrita de arquivos .obj
│   │   ├── export.c / .h     exportação do modelo deformado (.obj + .mtl + textura)
│   │   └── file_dialog.c / .h janela de abrir/salvar arquivo (Windows / zenity)
│   └── ui/
│       ├── ui.c / .h         painel Nuklear
│       └── nuklear_setup.h   opções comuns do Nuklear
├── shaders/
│   ├── morph.vert        mistura, deformações de Barr e normais numéricas
│   └── morph.frag        textura + iluminação de Phong
├── external/             glad (OpenGL 3.3), Nuklear, stb_image (terceiros)
├── docs/img/             capturas de exemplos de deformação
├── CMakeLists.txt
└── Makefile
```

---

## 7. Lembrete de entrega

O enunciado exige também a **lista da sequência de tarefas de cada membro**,
com data, hora de início e fim e tempo gasto (ex.: *"Fulano começou a resolver
um bug na função X às 23h do dia 22/01 até 5h do dia 23/01, levando
aproximadamente 4 horas"*). A falta dela causa perda de pontos.
