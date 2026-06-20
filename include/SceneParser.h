/**
 * SceneParser.h
 *
 * Parser simples para arquivos de cena JSON.
 * Lê configuração de objetos, luzes, materiais e trajetórias.
 *
 * Autor: aluno (amarqs182) - Unisinos 2026/1
 */

#ifndef SCENEPARSER_H
#define SCENEPARSER_H

#include <string>
#include <vector>
#include <glm/glm.hpp>

// Estrutura para material
struct Material {
    glm::vec3 color;
    float Ka;
    float Kd;
    float Ks;
    float shininess;
};

// Estrutura para trajetória
struct TrajectoryConfig {
    bool active;
    float speed;
    std::vector<glm::vec3> points;
};

// Estrutura para objeto
struct SceneObject {
    std::string id;
    std::string name;
    std::string type;  // "cube", "sphere", "pyramid"
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;
    Material material;
    std::string texture;
    TrajectoryConfig trajectory;
};

// Estrutura para luz
struct SceneLight {
    std::string id;
    std::string name;
    std::string type;  // "point"
    glm::vec3 position;
    glm::vec3 color;
    float intensity;
    bool active;
    glm::vec3 attenuation;  // constant, linear, quadratic
};

// Estrutura para câmera
struct CameraConfig {
    glm::vec3 position;
    float fov;
    float nearPlane;
    float farPlane;
};

// Estrutura para configuração da cena
struct SceneConfig {
    std::string name;
    glm::vec3 background;
    CameraConfig camera;
    std::vector<SceneObject> objects;
    std::vector<SceneLight> lights;
};

class SceneParser {
public:
    // Carrega cena de um arquivo JSON
    static SceneConfig loadFromFile(const std::string& filename);

    // Salva cena em um arquivo JSON
    static bool saveToFile(const SceneConfig& scene, const std::string& filename);

private {
    // Funções auxiliares de parsing
    static std::string readFile(const std::string& filename);
    static SceneConfig parseJSON(const std::string& json);
    static glm::vec3 parseVec3(const std::string& json, const std::string& key);
    static float parseFloat(const std::string& json, const std::string& key);
    static bool parseBool(const std::string& json, const std::string& key);
    static std::string parseString(const std::string& json, const std::string& key);
    static std::vector<glm::vec3> parseVec3Array(const std::string& json, const std::string& key);

    // Funções auxiliares de escrita
    static std::string vec3ToString(const glm::vec3& v);
    static std::string vec3ArrayToString(const std::vector<glm::vec3>& v);
    static std::string materialToString(const Material& m);
    static std::string trajectoryToString(const TrajectoryConfig& t);
};

#endif // SCENEPARSER_H
