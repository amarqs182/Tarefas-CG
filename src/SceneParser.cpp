/**
 * SceneParser.cpp
 *
 * Implementação do parser de cena JSON.
 *
 * Autor: aluno (amarqs182) - Unisinos 2026/1
 */

#include "SceneParser.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

std::string SceneParser::readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Erro ao abrir arquivo de cena: " << filename << std::endl;
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

glm::vec3 SceneParser::parseVec3(const std::string& json, const std::string& key) {
    glm::vec3 result(0.0f);

    size_t keyPos = json.find("\"" + key + "\"");
    if (keyPos == std::string::npos) return result;

    size_t bracketStart = json.find("[", keyPos);
    if (bracketStart == std::string::npos) return result;

    size_t bracketEnd = json.find("]", bracketStart);
    if (bracketEnd == std::string::npos) return result;

    std::string arrayContent = json.substr(bracketStart + 1, bracketEnd - bracketStart - 1);

    std::stringstream ss(arrayContent);
    std::string token;
    int i = 0;

    while (std::getline(ss, token, ',') && i < 3) {
        // Remove espaços em branco
        token.erase(0, token.find_first_not_of(" \t\n\r"));
        token.erase(token.find_last_not_of(" \t\n\r") + 1);

        result[i] = std::stof(token);
        i++;
    }

    return result;
}

float SceneParser::parseFloat(const std::string& json, const std::string& key) {
    size_t keyPos = json.find("\"" + key + "\"");
    if (keyPos == std::string::npos) return 0.0f;

    size_t colonPos = json.find(":", keyPos);
    if (colonPos == std::string::npos) return 0.0f;

    size_t valueStart = json.find_first_not_of(" \t\n\r", colonPos + 1);
    if (valueStart == std::string::npos) return 0.0f;

    size_t valueEnd = json.find_first_of(",}\n", valueStart);
    if (valueEnd == std::string::npos) valueEnd = json.length();

    std::string valueStr = json.substr(valueStart, valueEnd - valueStart);

    // Remove caracteres indesejados
    valueStr.erase(std::remove(valueStr.begin(), valueStr.end(), ' '), valueStr.end());
    valueStr.erase(std::remove(valueStr.begin(), valueStr.end(), '\t'), valueStr.end());
    valueStr.erase(std::remove(valueStr.begin(), valueStr.end(), '\n'), valueStr.end());
    valueStr.erase(std::remove(valueStr.begin(), valueStr.end(), '\r'), valueStr.end());

    try {
        return std::stof(valueStr);
    } catch (...) {
        return 0.0f;
    }
}

bool SceneParser::parseBool(const std::string& json, const std::string& key) {
    size_t keyPos = json.find("\"" + key + "\"");
    if (keyPos == std::string::npos) return false;

    size_t colonPos = json.find(":", keyPos);
    if (colonPos == std::string::npos) return false;

    size_t valueStart = json.find_first_not_of(" \t\n\r", colonPos + 1);
    if (valueStart == std::string::npos) return false;

    std::string valueStr = json.substr(valueStart, 5);
    std::transform(valueStr.begin(), valueStr.end(), valueStr.begin(), ::tolower);

    return valueStr.find("true") != std::string::npos;
}

std::string SceneParser::parseString(const std::string& json, const std::string& key) {
    size_t keyPos = json.find("\"" + key + "\"");
    if (keyPos == std::string::npos) return "";

    size_t colonPos = json.find(":", keyPos);
    if (colonPos == std::string::npos) return "";

    size_t quoteStart = json.find("\"", colonPos + 1);
    if (quoteStart == std::string::npos) return "";

    size_t quoteEnd = json.find("\"", quoteStart + 1);
    if (quoteEnd == std::string::npos) return "";

    return json.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
}

std::vector<glm::vec3> SceneParser::parseVec3Array(const std::string& json, const std::string& key) {
    std::vector<glm::vec3> result;

    size_t keyPos = json.find("\"" + key + "\"");
    if (keyPos == std::string::npos) return result;

    size_t bracketStart = json.find("[", keyPos);
    if (bracketStart == std::string::npos) return result;

    size_t bracketEnd = json.find("]", bracketStart);
    if (bracketEnd == std::string::npos) return result;

    std::string arrayContent = json.substr(bracketStart + 1, bracketEnd - bracketStart - 1);

    // Encontra cada sub-array [x, y, z]
    size_t pos = 0;
    while ((pos = arrayContent.find("[", pos)) != std::string::npos) {
        size_t end = arrayContent.find("]", pos);
        if (end == std::string::npos) break;

        std::string vecStr = arrayContent.substr(pos + 1, end - pos - 1);
        std::stringstream ss(vecStr);
        std::string token;
        glm::vec3 vec(0.0f);
        int i = 0;

        while (std::getline(ss, token, ',') && i < 3) {
            token.erase(0, token.find_first_not_of(" \t\n\r"));
            token.erase(token.find_last_not_of(" \t\n\r") + 1);
            vec[i] = std::stof(token);
            i++;
        }

        result.push_back(vec);
        pos = end + 1;
    }

    return result;
}

SceneConfig SceneParser::parseJSON(const std::string& json) {
    SceneConfig scene;

    // Parse scene info
    scene.name = parseString(json, "name");
    scene.background = parseVec3(json, "background");

    // Parse camera
    scene.camera.position = parseVec3(json, "position");
    scene.camera.fov = parseFloat(json, "fov");
    scene.camera.nearPlane = parseFloat(json, "near");
    scene.camera.farPlane = parseFloat(json, "far");

    // Parse objects (simplificado - encontra cada objeto)
    size_t objPos = 0;
    while ((objPos = json.find("\"type\": \"cube\"", objPos)) != std::string::npos ||
           (objPos = json.find("\"type\": \"sphere\"", objPos)) != std::string::npos ||
           (objPos = json.find("\"type\": \"pyramid\"", objPos)) != std::string::npos) {

        SceneObject obj;

        // Encontra o início do objeto (chave anterior)
        size_t objStart = json.rfind("{", objPos);
        if (objStart == std::string::npos) {
            objPos++;
            continue;
        }

        // Encontra o fim do objeto
        size_t objEnd = json.find("}", objPos);
        if (objEnd == std::string::npos) {
            objPos++;
            continue;
        }

        std::string objStr = json.substr(objStart, objEnd - objStart + 1);

        obj.id = parseString(objStr, "id");
        obj.name = parseString(objStr, "name");
        obj.type = parseString(objStr, "type");
        obj.position = parseVec3(objStr, "position");
        obj.rotation = parseVec3(objStr, "rotation");
        obj.scale = parseVec3(objStr, "scale");

        // Material
        obj.material.color = parseVec3(objStr, "color");
        obj.material.Ka = parseFloat(objStr, "Ka");
        obj.material.Kd = parseFloat(objStr, "Kd");
        obj.material.Ks = parseFloat(objStr, "Ks");
        obj.material.shininess = parseFloat(objStr, "shininess");

        // Texture
        obj.texture = parseString(objStr, "texture");

        // Trajectory
        obj.trajectory.active = parseBool(objStr, "active");
        obj.trajectory.speed = parseFloat(objStr, "speed");
        obj.trajectory.points = parseVec3Array(objStr, "points");

        scene.objects.push_back(obj);
        objPos = objEnd + 1;
    }

    // Parse lights
    size_t lightPos = 0;
    while ((lightPos = json.find("\"type\": \"point\"", lightPos)) != std::string::npos) {
        SceneLight light;

        size_t lightStart = json.rfind("{", lightPos);
        if (lightStart == std::string::npos) {
            lightPos++;
            continue;
        }

        size_t lightEnd = json.find("}", lightPos);
        if (lightEnd == std::string::npos) {
            lightPos++;
            continue;
        }

        std::string lightStr = json.substr(lightStart, lightEnd - lightStart + 1);

        light.id = parseString(lightStr, "id");
        light.name = parseString(lightStr, "name");
        light.type = parseString(lightStr, "type");
        light.position = parseVec3(lightStr, "position");
        light.color = parseVec3(lightStr, "color");
        light.intensity = parseFloat(lightStr, "intensity");
        light.active = parseBool(lightStr, "active");

        // Attenuation
        light.attenuation.x = parseFloat(lightStr, "constant");
        light.attenuation.y = parseFloat(lightStr, "linear");
        light.attenuation.z = parseFloat(lightStr, "quadratic");

        scene.lights.push_back(light);
        lightPos = lightEnd + 1;
    }

    return scene;
}

SceneConfig SceneParser::loadFromFile(const std::string& filename) {
    std::string json = readFile(filename);
    if (json.empty()) {
        SceneConfig empty;
        return empty;
    }
    return parseJSON(json);
}

// Funções de escrita
std::string SceneParser::vec3ToString(const glm::vec3& v) {
    return "[" + std::to_string(v.x) + ", " + std::to_string(v.y) + ", " + std::to_string(v.z) + "]";
}

std::string SceneParser::vec3ArrayToString(const std::vector<glm::vec3>& v) {
    std::string result = "[";
    for (size_t i = 0; i < v.size(); i++) {
        result += vec3ToString(v[i]);
        if (i < v.size() - 1) result += ", ";
    }
    result += "]";
    return result;
}

std::string SceneParser::materialToString(const Material& m) {
    return "{\n"
           "        \"color\": " + vec3ToString(m.color) + ",\n"
           "        \"Ka\": " + std::to_string(m.Ka) + ",\n"
           "        \"Kd\": " + std::to_string(m.Kd) + ",\n"
           "        \"Ks\": " + std::to_string(m.Ks) + ",\n"
           "        \"shininess\": " + std::to_string(m.shininess) + "\n"
           "      }";
}

std::string SceneParser::trajectoryToString(const TrajectoryConfig& t) {
    return "{\n"
           "        \"active\": " + std::string(t.active ? "true" : "false") + ",\n"
           "        \"speed\": " + std::to_string(t.speed) + ",\n"
           "        \"points\": " + vec3ArrayToString(t.points) + "\n"
           "      }";
}

bool SceneParser::saveToFile(const SceneConfig& scene, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Erro ao criar arquivo de cena: " << filename << std::endl;
        return false;
    }

    file << "{\n";
    file << "  \"scene\": {\n";
    file << "    \"name\": \"" << scene.name << "\",\n";
    file << "    \"background\": " << vec3ToString(scene.background) << "\n";
    file << "  },\n";

    file << "  \"objects\": [\n";
    for (size_t i = 0; i < scene.objects.size(); i++) {
        const SceneObject& obj = scene.objects[i];
        file << "    {\n";
        file << "      \"id\": \"" << obj.id << "\",\n";
        file << "      \"name\": \"" << obj.name << "\",\n";
        file << "      \"type\": \"" << obj.type << "\",\n";
        file << "      \"position\": " << vec3ToString(obj.position) << ",\n";
        file << "      \"rotation\": " << vec3ToString(obj.rotation) << ",\n";
        file << "      \"scale\": " << vec3ToString(obj.scale) << ",\n";
        file << "      \"material\": " << materialToString(obj.material) << ",\n";
        file << "      \"texture\": \"" << obj.texture << "\",\n";
        file << "      \"trajectory\": " << trajectoryToString(obj.trajectory) << "\n";
        file << "    }";
        if (i < scene.objects.size() - 1) file << ",";
        file << "\n";
    }
    file << "  ],\n";

    file << "  \"lights\": [\n";
    for (size_t i = 0; i < scene.lights.size(); i++) {
        const SceneLight& light = scene.lights[i];
        file << "    {\n";
        file << "      \"id\": \"" << light.id << "\",\n";
        file << "      \"name\": \"" << light.name << "\",\n";
        file << "      \"type\": \"" << light.type << "\",\n";
        file << "      \"position\": " << vec3ToString(light.position) << ",\n";
        file << "      \"color\": " << vec3ToString(light.color) << ",\n";
        file << "      \"intensity\": " << std::to_string(light.intensity) << ",\n";
        file << "      \"active\": " << std::string(light.active ? "true" : "false") << ",\n";
        file << "      \"attenuation\": {\n";
        file << "        \"constant\": " << std::to_string(light.attenuation.x) << ",\n";
        file << "        \"linear\": " << std::to_string(light.attenuation.y) << ",\n";
        file << "        \"quadratic\": " << std::to_string(light.attenuation.z) << "\n";
        file << "      }\n";
        file << "    }";
        if (i < scene.lights.size() - 1) file << ",";
        file << "\n";
    }
    file << "  ]\n";

    file << "}\n";
    file.close();

    return true;
}
