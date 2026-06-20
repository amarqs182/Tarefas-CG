/**
 * Trajectory.cpp
 *
 * Implementação da classe Trajectory para gerenciar trajetórias de objetos.
 *
 * Autor: aluno (amarqs182) - Unisinos 2026/1
 */

#include "Trajectory.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>

// Construtor
Trajectory::Trajectory()
    : currentPointIndex(0),
      currentPosition(0.0f, 0.0f, 0.0f),
      speed(1.0f),
      active(false),
      progress(0.0f)
{
}

// Adiciona um ponto de controle à trajetória
void Trajectory::addPoint(const glm::vec3& point)
{
    controlPoints.push_back(point);
}

// Remove um ponto de controle pelo índice
bool Trajectory::removePoint(size_t index)
{
    if (index >= controlPoints.size())
        return false;

    controlPoints.erase(controlPoints.begin() + index);

    // Ajusta o índice atual se necessário
    if (currentPointIndex >= controlPoints.size())
        currentPointIndex = 0;

    return true;
}

// Limpa todos os pontos
void Trajectory::clearPoints()
{
    controlPoints.clear();
    currentPointIndex = 0;
    progress = 0.0f;
}

// Retorna o número de pontos
size_t Trajectory::getPointCount() const
{
    return controlPoints.size();
}

// Retorna um ponto específico
glm::vec3 Trajectory::getPoint(size_t index) const
{
    if (index >= controlPoints.size())
        return glm::vec3(0.0f);

    return controlPoints[index];
}

// Retorna todos os pontos
const std::vector<glm::vec3>& Trajectory::getPoints() const
{
    return controlPoints;
}

// Move para o próximo ponto na trajetória (ciclicamente)
void Trajectory::advanceToNextPoint()
{
    if (controlPoints.empty())
        return;

    currentPointIndex = (currentPointIndex + 1) % controlPoints.size();
    progress = 0.0f;
}

// Retorna a posição atual (ponto de destino)
glm::vec3 Trajectory::getCurrentDestination() const
{
    if (controlPoints.empty())
        return glm::vec3(0.0f);

    return controlPoints[currentPointIndex];
}

// Retorna a posição atual do objeto (interpolação linear simples)
glm::vec3 Trajectory::getCurrentPosition() const
{
    if (controlPoints.empty())
        return glm::vec3(0.0f);

    if (controlPoints.size() == 1)
        return controlPoints[0];

    // Calcula o ponto anterior
    size_t prevIndex = (currentPointIndex == 0) 
        ? controlPoints.size() - 1 
        : currentPointIndex - 1;

    // Interpola entre o ponto anterior e o atual
    return lerp(controlPoints[prevIndex], controlPoints[currentPointIndex], progress);
}

// Atualiza a posição do objeto (interpolação linear)
void Trajectory::update(float deltaTime)
{
    if (!active || controlPoints.empty())
        return;

    // Calcula a distância entre os pontos
    size_t prevIndex = (currentPointIndex == 0) 
        ? controlPoints.size() - 1 
        : currentPointIndex - 1;

    float distance = glm::length(controlPoints[currentPointIndex] - controlPoints[prevIndex]);

    // Evita divisão por zero
    if (distance < 0.0001f)
    {
        advanceToNextPoint();
        return;
    }

    // Calcula o tempo para percorrer a distância
    float travelTime = distance / speed;

    // Atualiza o progresso
    progress += deltaTime / travelTime;

    // Se completou o trecho, avança para o próximo ponto
    if (progress >= 1.0f)
    {
        progress = 0.0f;
        advanceToNextPoint();
    }
}

// Define a velocidade de movimento
void Trajectory::setSpeed(float speed)
{
    this->speed = speed;
}

// Retorna a velocidade atual
float Trajectory::getSpeed() const
{
    return speed;
}

// Verifica se a trajetória está ativa
bool Trajectory::isActive() const
{
    return active;
}

// Ativa/desativa a trajetória
void Trajectory::setActive(bool active)
{
    this->active = active;
}

// Reseta a trajetória para o início
void Trajectory::reset()
{
    currentPointIndex = 0;
    progress = 0.0f;
    if (!controlPoints.empty())
        currentPosition = controlPoints[0];
}

// Salva os pontos em um arquivo
bool Trajectory::saveToFile(const std::string& filename) const
{
    std::ofstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "Erro ao abrir arquivo para escrita: " << filename << std::endl;
        return false;
    }

    // Salva o número de pontos
    file << controlPoints.size() << std::endl;

    // Salva cada ponto
    for (const auto& point : controlPoints)
    {
        file << point.x << " " << point.y << " " << point.z << std::endl;
    }

    file.close();
    return true;
}

// Carrega pontos de um arquivo
bool Trajectory::loadFromFile(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "Erro ao abrir arquivo para leitura: " << filename << std::endl;
        return false;
    }

    // Limpa os pontos atuais
    clearPoints();

    // Lê o número de pontos
    size_t numPoints;
    file >> numPoints;

    // Lê cada ponto
    for (size_t i = 0; i < numPoints; ++i)
    {
        float x, y, z;
        file >> x >> y >> z;
        controlPoints.push_back(glm::vec3(x, y, z));
    }

    file.close();

    // Reseta para o início
    reset();

    return true;
}

// Retorna o progresso atual (0.0 a 1.0)
float Trajectory::getProgress() const
{
    return progress;
}

// Retorna o índice do ponto atual
size_t Trajectory::getCurrentIndex() const
{
    return currentPointIndex;
}

// Interpolação linear entre dois pontos
glm::vec3 Trajectory::lerp(const glm::vec3& a, const glm::vec3& b, float t) const
{
    // Clamp do t entre 0 e 1
    t = std::max(0.0f, std::min(1.0f, t));
    return a + t * (b - a);
}