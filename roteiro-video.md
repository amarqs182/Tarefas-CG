# Roteiro de Video - Diorama 3D (3-4 minutos)

## Pre-gravacao
- Compilar o projeto antes de gravar
- Ter o terminal e IDE abertos
- OBS configurado para capturar tela + audio
- Fechar outros programas para evitar lag

---

## 1. Introducao (20s)

**Narrar:**
> "Ola, meu nome e [seu nome] e este e o projeto final da disciplina de Computacao Grafica 2026/1 da Unisinos. O projeto e um visualizador 3D completo, ou diorama, que integra todos os topicos vistos ao longo do semestre: OpenGL, camera, iluminacao Phong, curvas de Bezier, texturas e carregamento de modelos .obj."

**Mostrar:** Janela do MainDiorama aberta com a cena visivel (cubo, esfera, piramide, chao, Suzanne).

---

## 2. Arquitetura (30s)

**Narrar:**
> "A aplicacao foi desenvolvida em C++17 com OpenGL 4.x. O codigo principal esta em MainDiorama.cpp, que integra todos os modulos: Camera para navegacao, ObjParser para carregar modelos .obj com materiais .mtl, e a interface ImGui para controle em tempo real. As dependencias GLFW, GLM, stb_image e ImGui sao baixadas automaticamente pelo CMake FetchContent."

**Mostrar:** Abrir o codigo no VS Code e mostrar rapidamente a estrutura de pastas (src/, include/, assets/).

---

## 3. Demonstracao de Funcionalidades (2 min)

### 3.1 Camera (15s)
**Narrar:**
> "A camera e do tipo FPS, controlada por WASD para mover, Q e E para subir e descer, mouse para olhar em volta e scroll para zoom."

**Acao:** Mover a camera pela cena usando WASD + mouse.

### 3.2 Selecao e Transformacao (20s)
**Narrar:**
> "Posso selecionar objetos com TAB eMove-los com IJKL, rotacionar com U e O, e escalar com mais e menos."

**Acao:** Selecionar o cubo (TAB), move-lo (IJKL), rotacionar (U), escalar (+).

### 3.3 Iluminacao Phong (20s)
**Narrar:**
> "A iluminacao usa o modelo Phong com 3 luzes pontuais: Key Light, Fill Light e Back Light. Posso ligar e desligar cada uma individualmente com F1, F2 e F3, ou todas com F4."

**Acao:** Pressionar F1 (desligar luz principal), F2 (desligar luz secundaria), mostrar a diferenca, F4 (ligar todas).

### 3.4 Materiais e Texturas (20s)
**Narrar:**
> "O painel ImGui permite editar o material do objeto selecionado: cor, Ka, Kd, Ks e shininess. Alem disso, posso trocar a textura em tempo real."

**Acao:** Abrir o ImGui, mudar a cor do cubo, ajustar Ka/Kd/Ks, aplicar a textura pixelWall na esfera.

### 3.5 Curvas de Bezier (20s)
**Narrar:**
> "Os objetos podem percorrer trajetorias definidas por curvas de Bezier. Ativo com SPACE e alterno o modo com B. Os pontos de controle sao adicionados com 1 e removidos com 2."

**Acao:** Ativar trajetoria do cubo (SPACE), mostrar ele se movendo, alternar modo Bezier (B).

### 3.6 Carregamento .obj (15s)
**Narrar:**
> "O modelo Suzanne e carregado de um arquivo .obj com seu respectivo .mtl. O parser extrai vertices, normais, coordenadas de textura e materiais, incluindo a textura automatica."

**Acao:** Selecionar o Suzanne com TAB, mostrar que ele tem textura propria do .mtl.

---

## 4. Codigo-Fonte (30s)

**Narrar:**
> "Alguns pontos importantes do codigo:"

**Mostrar no VS Code:**
1. **Scene setup** (~linha 800): onde os objetos e luzes sao configurados
2. **Shader Phong** (~linha 540): o calculo de iluminacao no fragment shader (Ka, Kd, Ks)
3. **Matriz Model** (~linha 627): a funcao transformacaoGenerica que aplica translate, rotate, scale
4. **ObjParser** (~linha 878): onde o .obj e carregado e os materiais extraidos

---

## 5. Conclusao (10s)

**Narrar:**
> "Este projeto consolidou meus conhecimentos em OpenGL, desde a renderizacao basica ate a integracao completa com iluminacao, texturas e interface grafica. Obrigado pela atencao."

**Mostrar:** Cena completa do diorama.

---

## Dicas de Gravacao
- Fale devagar e com calma
- Nao corra nas demonstracoes - mostre cada funcionalidade com calma
- Se errar algo, continue gravando e corrija na edicao
- Grave em 1080p se possivel
- Teste o audio antes de comecar
