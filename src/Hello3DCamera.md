# Tarefa 4 — Câmera em primeira pessoa

**Disciplina:** Computação Gráfica · Unisinos · 2026/1
**Prazo:** 02/06/2026, 23:59 (BRT)
**Autor:** amarqs182

## Enunciado

> Implemente uma câmera em primeira pessoa. Para isso, recomendamos seguir as
> instruções apresentadas no material de aprofundamento. Sugerimos que vocês
> implementem a câmera como um objeto de uma classe **Câmera**, de maneira a
> agrupar todos os seus atributos e encapsular as ações de **Mover** e
> **Rotacionar**.

## Arquivos entregues

| Arquivo | Descrição |
|---------|-----------|
| [`include/Camera.h`](../../include/Camera.h) | Header da classe `Camera` |
| [`src/Camera.cpp`](../../Camera.cpp) | Implementação da classe `Camera` |
| [`src/Hello3DCamera.cpp`](../../Hello3DCamera.cpp) | Programa principal: cubo + câmera FPS |
| [`../../CMakelists.txt`](../../CMakelists.txt) | Adicionado `Hello3DCamera` à lista `EXERCISES` |

## Classe `Camera` — API pública

### Atributos (encapsulados)

```cpp
glm::vec3 position;     // posição no mundo
glm::vec3 front;        // vetor direção (calculado de yaw/pitch)
glm::vec3 up, right;    // vetores up/right recalculados
glm::vec3 worldUp;      // (0,1,0) por padrão

float yaw, pitch;       // ângulos de Euler (graus)
float movementSpeed;    // 2.5 por padrão
float mouseSensitivity; // 0.05 por padrão
float fov;              // 45° por padrão

float aspect, nearPlane_, farPlane_;  // parâmetros de projeção
```

### Métodos

```cpp
Camera(glm::vec3 position = {0,0,3}, glm::vec3 up = {0,1,0},
       float yaw = -90, float pitch = 0);

glm::mat4 getViewMatrix() const;
glm::mat4 getProjectionMatrix() const;

void processKeyboard(Direction dir, float deltaTime);   // Mover
void processMouseMovement(float xoff, float yoff);      // Rotacionar
void processMouseScroll(float yoff);                    // Zoom

void setProjectionParams(float fovDeg, float aspect,
                         float nearPlane, float farPlane);
```

A enum `Direction` tem 6 valores: `FORWARD, BACKWARD, LEFT, RIGHT_DIR, UP, DOWN`.

## Controles no programa

| Tecla | Ação |
|-------|------|
| W / S | Mover para frente / trás (encapsulado em `processKeyboard`) |
| A / D | Strafe esquerda / direita |
| Q / E | Descer / subir (eixo Y mundo) |
| Mouse | Olhar em volta — yaw e pitch (encapsulado em `processMouseMovement`) |
| Scroll | Zoom — altera FOV entre 1° e 45° |
| ESC | Sair |

## Matemática implementada

- **View:** `glm::lookAt(position, position + front, up)`
- **Front (vetor direção):**  
  `front.x = cos(p) * cos(y)`  
  `front.y = sin(p)`  
  `front.z = cos(p) * sin(y)`  
  (com `p = pitch`, `y = yaw` em radianos)
- **Right:** `normalize(cross(front, worldUp))`
- **Up:** `normalize(cross(right, front))`
- **Projection:** `glm::perspective(fov, aspect, near, far)`
- **Pitch:** clamped em ±89° para evitar gimbal lock

## Como compilar e executar

```bash
cd CGCCHibrido
mkdir build && cd build
cmake ..
make Hello3DCamera    # ou: cmake --build . --target Hello3DCamera
./Hello3DCamera       # Linux/macOS
# ou Hello3DCamera.exe no Windows
```

## Decisões de design

1. **Câmera como classe única**, não free functions: cumpre o requisito do enunciado
   de "agrupar todos os atributos e encapsular as ações de Mover e Rotacionar".
2. **Dois métodos de "Mover" e "Rotacionar" distintos**: `processKeyboard()` e
   `processMouseMovement()`. O nome `Rotacionar` foi preferido em vez de
   `RotacionarYaw` para manter a API limpa; a rotação é totalmente controlada
   por esses dois métodos.
3. **Padrão LearnOpenGL** para as fórmulas (yaw = -90° inicial = olhando para -Z).
4. **Gimbal lock evitado** com clamp de pitch em ±89°.
5. **deltaTime** para a velocidade ficar independente do framerate.
6. **Cubo colorido** (cada face de uma cor) para confirmar visualmente que a
   câmera está realmente se movendo — sem isso, com cor única, daria a impressão
   de que a câmera está parada.

## Compatibilidade com Tarefas anteriores

Esta entrega parte do mesmo projeto das tarefas 1 (ambiente) e 2 (cubos com cores
diferentes e múltiplos cubos). O cubo desenhado aqui usa o mesmo esquema de cores
da Tarefa 2 — uma cor por face.

## Limitações conhecidas

- Sem quaternion: pode ocorrer gimbal lock em rotações extremas. O próprio
  material de aprofundamento cita a solução (capítulo 17 do opengl-tutorial.org).
- Câmera é usada em mundo "vazio" (apenas um cubo central). Para testar
  completamente a navegação, basta replicar o cubo em diferentes posições
  (coisa que a Tarefa 2 já ensinou).
