/**
 * Camera.cpp - Implementação da classe Camera
 *
 * Câmera sintética em primeira pessoa baseada no material do Módulo 5
 * (Câmera Sintética, Profa. Rossana B Queiroz) e LearnOpenGL.com.
 *
 * Matemática:
 *   - view = lookAt(position, position + front, up)
 *   - front (vetor direção) derivado de yaw/pitch:
 *       front.x = cos(pitch) * cos(yaw)
 *       front.y = sin(pitch)
 *       front.z = cos(pitch) * sin(yaw)
 *   - right = normalize(cross(front, worldUp))
 *   - up    = normalize(cross(right, front))
 *   - projection = perspective(fov, aspect, near, far)
 */

#include "Camera.h"

#include <cmath>

// Inicialização das constantes estáticas
const glm::vec3 Camera::WORLD_UP    = glm::vec3(0.0f, 1.0f, 0.0f);
const glm::vec3 Camera::WORLD_FRONT = glm::vec3(0.0f, 0.0f, -1.0f);
const glm::vec3 Camera::WORLD_RIGHT = glm::vec3(1.0f, 0.0f, 0.0f);

// Construtor com vetores
Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch)
    : position(position)
    , front(glm::vec3(0.0f, 0.0f, -1.0f))
    , up(glm::vec3(0.0f, 1.0f, 0.0f))
    , right(glm::vec3(1.0f, 0.0f, 0.0f))
    , worldUp(up)
    , yaw(yaw)
    , pitch(pitch)
    , movementSpeed(2.5f)
    , mouseSensitivity(0.05f)
    , fov(45.0f)
    , aspect(800.0f / 600.0f)
    , nearPlane_(0.1f)
    , farPlane_(100.0f)
{
    updateCameraVectors();
}

// Construtor com escalares
Camera::Camera(float posX, float posY, float posZ,
               float upX, float upY, float upZ,
               float yaw, float pitch)
    : Camera(glm::vec3(posX, posY, posZ),
             glm::vec3(upX, upY, upZ),
             yaw, pitch)
{
}

glm::mat4 Camera::getViewMatrix() const
{
    // view = lookAt(eye, center, up)
    return glm::lookAt(position, position + front, up);
}

glm::mat4 Camera::getProjectionMatrix() const
{
    return glm::perspective(glm::radians(fov), aspect, nearPlane_, farPlane_);
}

void Camera::processKeyboard(Direction direction, float deltaTime)
{
    float velocity = movementSpeed * deltaTime;

    if (direction == FORWARD)
        position += front * velocity;
    if (direction == BACKWARD)
        position -= front * velocity;
    if (direction == LEFT)
        position -= right * velocity;
    if (direction == RIGHT_DIR)
        position += right * velocity;
    if (direction == UP)
        position += worldUp * velocity;
    if (direction == DOWN)
        position -= worldUp * velocity;
}

void Camera::processMouseMovement(float xoffset, float yoffset,
                                  bool constrainPitch)
{
    xoffset *= mouseSensitivity;
    yoffset *= mouseSensitivity;

    yaw   += xoffset;
    pitch += yoffset;

    // Evita "Gimbal lock": pitch limitado a ±89°
    if (constrainPitch)
    {
        if (pitch > 89.0f)  pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;
    }

    updateCameraVectors();
}

void Camera::processMouseScroll(float yoffset)
{
    fov -= yoffset;
    if (fov < 1.0f)  fov = 1.0f;
    if (fov > 45.0f) fov = 45.0f;
}

void Camera::updateCameraVectors()
{
    // Calcula o novo vetor front
    glm::vec3 f;
    f.x = cos(glm::radians(pitch)) * cos(glm::radians(yaw));
    f.y = sin(glm::radians(pitch));
    f.z = cos(glm::radians(pitch)) * sin(glm::radians(yaw));
    front = glm::normalize(f);

    // Recalcula right e up
    right = glm::normalize(glm::cross(front, worldUp));
    up    = glm::normalize(glm::cross(right, front));
}
