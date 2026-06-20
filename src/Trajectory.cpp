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
      currentSegment(0),
      currentPosition(0.0f, 0.0f, 0.0f),
      speed(1.0f),
      active(false),
      progress(0.0f),
      interpolation(Interpolation::LINEAR)
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

// Retorna a posição atual do objeto (usa computeCurrentPosition que despacha
// entre interpolação linear e bezier cubico baseado em `interpolation`)
glm::vec3 Trajectory::getCurrentPosition() const
{
    return computeCurrentPosition();
}

// Atualiza a posição do objeto (linear OU bezier cubico, baseado em `interpolation`)
void Trajectory::update(float deltaTime)
{
    if (!active) return;

    if (interpolation == Interpolation::LINEAR) {
        if (controlPoints.empty()) return;

        // Calcula a distância entre os pontos
        size_t prevIndex = (currentPointIndex == 0)
            ? controlPoints.size() - 1
            : currentPointIndex - 1;

        float distance = glm::length(controlPoints[currentPointIndex] - controlPoints[prevIndex]);

        // Evita divisão por zero
        if (distance < 0.0001f) {
            advanceToNextPoint();
            return;
        }

        // Calcula o tempo para percorrer a distância
        float travelTime = distance / speed;

        // Atualiza o progresso
        progress += deltaTime / travelTime;

        // Se completou o trecho, avança para o próximo ponto
        if (progress >= 1.0f) {
            progress = 0.0f;
            advanceToNextPoint();
        }
    } else {
        // BEZIER cubico
        if (bezierPoints.size() < 2) return;

        // Numero de segmentos cubicos = ceil((N-1)/3), minimo 1
        size_t numSegments = (bezierPoints.size() >= 4)
            ? ((bezierPoints.size() - 1) / 3 + 1)
            : 1;
        if (numSegments < 1) numSegments = 1;

        progress += deltaTime * speed * 0.2f;

        if (progress >= 1.0f) {
            progress = 0.0f;
            currentSegment++;
            if (currentSegment >= numSegments) {
                currentSegment = 0;
            }
        }
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

// ============================================================================
// Bezier cubico (adicionado em refactor/use-trajectory-class)
// Cada segmento cubico usa 4 pontos de controle P0, P1, P2, P3.
// O usuario pode digitar N pontos onde N = 3k + 1 -> k segmentos cubicos.
// ============================================================================

// === Configurar tipo de interpolacao ===
void Trajectory::setInterpolation(Interpolation type) {
    interpolation = type;
    progress = 0.0f;
}
Trajectory::Interpolation Trajectory::getInterpolation() const {
    return interpolation;
}

// === Gestao dos pontos bezier ===
void Trajectory::addBezierPoint(const glm::vec3& point) {
    bezierPoints.push_back(point);
}
bool Trajectory::removeBezierPoint(size_t index) {
    if (index >= bezierPoints.size()) return false;
    bezierPoints.erase(bezierPoints.begin() + index);
    if (currentSegment > 0 && currentSegment * 3 >= bezierPoints.size()) currentSegment = 0;
    return true;
}
void Trajectory::clearBezierPoints() {
    bezierPoints.clear();
    currentSegment = 0;
    progress = 0.0f;
}
const std::vector<glm::vec3>& Trajectory::getBezierPoints() const {
    return bezierPoints;
}
size_t Trajectory::getBezierPointCount() const {
    return bezierPoints.size();
}
void Trajectory::setCurrentSegment(size_t seg) {
    currentSegment = seg;
}
size_t Trajectory::getCurrentSegment() const {
    return currentSegment;
}

// === De Casteljau sobre 4 pontos de controle ===
glm::vec3 Trajectory::bezierCubic(const glm::vec3& p0, const glm::vec3& p1,
                                  const glm::vec3& p2, const glm::vec3& p3, float t) {
    // Metodo classico: reduz 4 -> 3 -> 2 -> 1 pontos por iteracao
    glm::vec3 a = glm::mix(p0, p1, t);
    glm::vec3 b = glm::mix(p1, p2, t);
    glm::vec3 c = glm::mix(p2, p3, t);
    glm::vec3 d = glm::mix(a, b, t);
    glm::vec3 e = glm::mix(b, c, t);
    return glm::mix(d, e, t);
}

// === Salva/carrega bezier ===
bool Trajectory::saveBezierToFile(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) return false;
    file << bezierPoints.size() << "\n";
    for (const auto& p : bezierPoints) {
        file << p.x << " " << p.y << " " << p.z << "\n";
    }
    return true;
}
bool Trajectory::loadBezierFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return false;
    clearBezierPoints();
    size_t n; file >> n;
    for (size_t i = 0; i < n; ++i) {
        float x, y, z; file >> x >> y >> z;
        bezierPoints.push_back(glm::vec3(x, y, z));
    }
    return true;
}

// === Calculo da posicao baseado no tipo de interpolacao ===
glm::vec3 Trajectory::computeCurrentPosition() const {
    if (interpolation == Interpolation::LINEAR) {
        if (controlPoints.empty()) return currentPosition;
        if (controlPoints.size() == 1) return controlPoints[0];
        size_t prevIndex = (currentPointIndex == 0)
            ? controlPoints.size() - 1
            : currentPointIndex - 1;
        return lerp(controlPoints[prevIndex], controlPoints[currentPointIndex], progress);
    }
    // BEZIER cubico
    if (bezierPoints.empty()) return currentPosition;
    if (bezierPoints.size() < 2) return bezierPoints[0];

    size_t baseIdx = currentSegment * 3;
    if (baseIdx + 3 >= bezierPoints.size()) {
        // Segmento incompleto — usa os 4 primeiros pontos (fallback)
        baseIdx = 0;
    }
    return bezierCubic(
        bezierPoints[baseIdx + 0],
        bezierPoints[baseIdx + 1],
        bezierPoints[baseIdx + 2],
        bezierPoints[baseIdx + 3],
        progress);
}