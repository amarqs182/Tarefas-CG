/**
 * Trajectory.h
 *
 * Classe para gerenciar trajetórias de objetos em cena 3D.
 * Permite adicionar pontos de controle e mover objetos ciclicamente
 * por esses pontos.
 *
 * Baseado na tarefa: "Definindo trajetórias para alguns objetos"
 * - Adicionar pontos no espaço e armazenar coordenadas
 * - Translação cíclica pelos pontos (ao chegar no último, volta ao primeiro)
 * - Sem interpolação por curva cúbica (será na Atividade Vivencial)
 *
 * Autor: aluno (amarqs182) - Unisinos 2026/1
 */

#ifndef TRAJECTORY_H
#define TRAJECTORY_H

#include <vector>
#include <glm/glm.hpp>
#include <string>

class Trajectory
{
public:
    // Construtor
    Trajectory();

    // Adiciona um ponto de controle à trajetória
    void addPoint(const glm::vec3& point);

    // Remove um ponto de controle pelo índice
    bool removePoint(size_t index);

    // Limpa todos os pontos
    void clearPoints();

    // Retorna o número de pontos
    size_t getPointCount() const;

    // Retorna um ponto específico
    glm::vec3 getPoint(size_t index) const;

    // Retorna todos os pontos
    const std::vector<glm::vec3>& getPoints() const;

    // Move para o próximo ponto na trajetória (ciclicamente)
    void advanceToNextPoint();

    // Retorna a posição atual (ponto de destino)
    glm::vec3 getCurrentDestination() const;

    // Retorna a posição atual do objeto (interpolação linear simples)
    glm::vec3 getCurrentPosition() const;

    // Atualiza a posição do objeto (interpolação linear)
    void update(float deltaTime);

    // Define a velocidade de movimento
    void setSpeed(float speed);

    // Retorna a velocidade atual
    float getSpeed() const;

    // Verifica se a trajetória está ativa
    bool isActive() const;

    // Ativa/desativa a trajetória
    void setActive(bool active);

    // Reseta a trajetória para o início
    void reset();

    // Salva os pontos em um arquivo
    bool saveToFile(const std::string& filename) const;

    // Carrega pontos de um arquivo
    bool loadFromFile(const std::string& filename);

    // Retorna o progresso atual (0.0 a 1.0)
    float getProgress() const;

    // Retorna o índice do ponto atual
    size_t getCurrentIndex() const;

private:
    std::vector<glm::vec3> controlPoints;  // Lista de pontos de controle
    size_t currentPointIndex;              // Índice do ponto atual
    glm::vec3 currentPosition;             // Posição atual do objeto
    float speed;                           // Velocidade de movimento
    bool active;                           // Se a trajetória está ativa
    float progress;                        // Progresso entre pontos (0.0 a 1.0)

    // Interpolação linear entre dois pontos
    glm::vec3 lerp(const glm::vec3& a, const glm::vec3& b, float t) const;
};

#endif // TRAJECTORY_H