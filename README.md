# Diorama 3D - Avaliacao Final

Projeto final da disciplina de Computacao Grafica (2026/1) - Unisinos.
Visualizador 3D completo integrando todos os topicos do semestre: OpenGL, camera FPS, iluminacao Phong, curvas de Bezier, texturas, carregamento de modelos .obj e interface ImGui.

## Funcionalidades

| Feature | Descricao | Atalho |
|---------|-----------|--------|
| Camera FPS | Movimento livre com WASD/QE + mouse + scroll | WASD/QE, Mouse, Scroll |
| Transformacoes | Translacao, rotacao e escala por objeto | TAB, IJKL, U/O, +/- |
| Iluminacao Phong | 3 luzes pontuais (Key, Fill, Back) | F1/F2/F3 (individual), F4 (todas) |
| Curvas de Bezier | Trajetorias com interpolacao linear e Bezier | SPACE, B, 1-5, R |
| Texturas | Troca automatica (.mtl) e manual via ImGui | Painel ImGui |
| Modelos .obj | Carregamento com materiais e normais | Automatico |
| ImGui | Painel de controle para todos os parametros | Janela flutuante |

## Arquivos

```
Tarefas-CG/
├── src/
│   ├── MainDiorama.cpp        # Aplicacao principal (diorama)
│   ├── Camera.cpp             # Camera FPS
│   ├── ObjParser.cpp          # Parser de .obj/.mtl
│   ├── Trajectory.cpp         # Curvas de Bezier
│   ├── SceneParser.cpp        # Leitura de cena JSON
│   ├── Hello3D.cpp            # Exercicio: triangulo basico
│   ├── Hello3DCamera.cpp      # Exercicio: camera simples
│   ├── TriangleTex.cpp        # Exercicio: textura
│   ├── SpherePhong.cpp        # Exercicio: esfera + Phong
│   ├── CuboIluminacao.cpp     # Exercicio: cubo iluminado
│   └── TrajectoryDemo.cpp     # Exercicio: trajetorias
├── include/
│   ├── Camera.h
│   ├── ObjParser.h
│   ├── SceneParser.h
│   └── Trajectory.h
├── Common/
│   └── glad.c                 # GLAD loader
├── assets/
│   ├── Modelos3D/             # Modelos .obj e .mtl
│   │   ├── Suzanne.obj/.mtl/.png
│   │   └── Cube.obj/.mtl
│   ├── tex/                   # Texturas
│   │   └── pixelWall.png
│   └── scene.json             # Configuracao da cena
├── CMakeLists.txt
├── GettingStarted.md          # Tutorial de setup do ambiente
├── apresentacao.pptx          # Apresentacao da avaliacao (13 slides)
└── roteiro-video.md           # Roteiro para gravacao do video
```

## Setup

### Pre-requisitos

- Compilador C++17 (GCC/MinGW ou Clang)
- CMake 3.10+
- Git (para FetchContent baixar dependencias)
- OpenGL 4.1+

### Build

```bash
# Clonar
git clone https://github.com/amarqs182/Tarefas-CG.git
cd Tarefas-CG

# Compilar o projeto final
mkdir build && cd build
cmake ..
cmake --build . --target MainDiorama

# Rodar
./MainDiorama
```

### GLAD

Os arquivos da GLAD devem ser baixados manualmente:
1. Acesse [GLAD Generator](https://glad.dav1d.de/)
2. Configuracao: OpenGL 4.1+, Core Profile, C/C++
3. Coloque `glad.h` em `include/glad/`, `khrplatform.h` em `include/glad/KHR/`, `glad.c` em `Common/`

## Assets

### Modelos 3D

| Modelo | Procedencia | Licenca |
|--------|-------------|---------|
| Suzanne | Blender (open source) | CC-BY |
| Cube | Gerado proceduralmente | - |

### Texturas

| Textura | Procedencia | Licenca |
|---------|-------------|---------|
| pixelWall.png | https://ambientcg.com/ | CC0 |
| Suzanne.png | Incluida com o modelo Suzanne | CC-BY |

### Configuracao da Cena

O arquivo `scene.json` define os objetos e luzes da cena. Tambem e possivel configurar diretamente no codigo (`inicializaCena()`).

## Referencias

### Documentacao

- [LearnOpenGL](https://learnopengl.com/) - Tutoriais de OpenGL
- [OpenGL Documentation](https://docs.gl/) - Referencia oficial do OpenGL
- [GLM Documentation](https://glm.g-truc.net/) - Biblioteca de matematica
- [ImGui Documentation](https://github.com/ocornut/imgui) - Interface grafica
- [GLAD Generator](https://glad.dav1d.de/) - Gerador de OpenGL loader

### Bibliografia

- Material de aula: Computacao Grafica 2026/1 - Unisinos
- CMake FetchContent: https://cmake.org/cmake/help/latest/module/FetchContent.html

### Sugerir Texturas e Modelos

- https://ambientcg.com/
- https://polyhaven.com/textures
- https://sketchfab.com/features/free-3d-models
- https://free3d.com/

## Controles Detalhados

### Camera
- **W/S** - Frente/tras
- **A/D** - Esquerda/direita
- **Q/E** - Baixo/cima
- **Mouse** - Olhar em volta
- **Scroll** - Zoom

### Objeto Selecionado
- **TAB** - Trocar objeto
- **I/K** - Transladar Z
- **J/L** - Transladar X
- **U/O** - Rotacionar Y
- **+/--** - Escalar

### Iluminacao
- **F1/F2/F3** - Lig/deslig luz 1/2/3
- **F4** - Lig/deslig todas

### Trajetoria
- **SPACE** - Ativar/desativar trajetoria
- **B** - Alternar modo Bezier
- **1** - Adicionar ponto de controle
- **2** - Remover ultimo ponto
- **3** - Limpar pontos
- **4/5** - Adicionar/remover ponto Bezier
- **R** - Resetar trajetoria
- **UP/DOWN** - Ajustar velocidade

## Autor

- amarqs182 - Unisinos 2026/1
