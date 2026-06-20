/**
 * Camera.h
 *
 * Câmera sintética em primeira pessoa (FPS-style).
 * Baseada no material de aprofundamento do Módulo 5 (Câmera Sintética)
 * e na especificação da Tarefa 4 (Adicionando uma câmera em primeira pessoa).
 *
 * Encapsula:
 *   - Atributos: posição, orientação (yaw/pitch), parâmetros de projeção
 *   - Movimentação: WASD (XZ), Q/E (Y), com deltaTime
 *   - Rotação: mouse (clamp pitch ±89°)
 *   - Zoom: scroll (clamp FOV 1°..45°)
 *
 * Uso típico:
 *   Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
 *   ...
 *   view = camera.getViewMatrix();
 *   projection = camera.getProjectionMatrix();
 *   camera.processKeyboard(Camera::FORWARD, deltaTime);
 *   camera.processMouseMovement(xoffset, yoffset);
 *
 * Autor: aluno (amarqs182) - Unisinos 2026/1
 */

#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera
{
public:
    // Vetores padrão (constantes do mundo)
    static const glm::vec3 WORLD_UP;
    static const glm::vec3 WORLD_FRONT;
    static const glm::vec3 WORLD_RIGHT;

    // Direções de movimento para processKeyboard
    enum Direction {
        FORWARD,
        BACKWARD,
        LEFT,
        RIGHT_DIR,
        UP,
        DOWN
    };

    // Construtor com vetores default
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f),
           glm::vec3 up       = glm::vec3(0.0f, 1.0f, 0.0f),
           float yaw          = -90.0f,
           float pitch        = 0.0f);

    // Construtor com escalares
    Camera(float posX, float posY, float posZ,
           float upX, float upY, float upZ,
           float yaw, float pitch);

    // Matrizes
    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix() const;

    // Getters
    glm::vec3 getPosition() const { return position; }
    glm::vec3 getFront()    const { return front; }
    float     getFOV()      const { return fov; }

    // Processa input do teclado. deltaTime em segundos.
    // direction: FORWARD/BACKWARD/LEFT/RIGHT_DIR/UP/DOWN
    void processKeyboard(Direction direction, float deltaTime);

    // Processa movimento do mouse. xoffset/yoffset em pixels (já com sinal).
    // O método aplica a sensibilidade e o clamp de pitch.
    void processMouseMovement(float xoffset, float yoffset,
                              bool constrainPitch = true);

    // Processa scroll do mouse (zoom). yoffset é o valor do GLFW.
    void processMouseScroll(float yoffset);

    // Setters de parâmetros de projeção
    void setProjectionParams(float fovDeg, float aspect,
                             float nearPlane, float farPlane)
    {
        fov = fovDeg;
        aspect = aspect;
        nearPlane_ = nearPlane;
        farPlane_ = farPlane;
    }

    // Parâmetros ajustáveis
    float movementSpeed;     // unidades/segundo
    float mouseSensitivity;  // multiplicador do deslocamento
    float fov;               // graus

private:
    // Atributos de posição e orientação
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;

    // Ângulos de Euler (em graus)
    float yaw;
    float pitch;

    // Parâmetros de projeção
    float aspect;
    float nearPlane_;
    float farPlane_;

    // Recalcula front, right, up a partir de yaw/pitch.
    // Chamado quando yaw/pitch mudam.
    void updateCameraVectors();
};

#endif // CAMERA_H
