/**
 * ObjParser.h
 *
 * Parser para arquivos .obj (Wavefront) com suporte a .mtl.
 * Suporta: vértices (v), normais (vn), coordenadas de textura (vt), faces (f),
 *          materiais (mtllib, usemtl), e propriedades de material (Kd, Ks, etc).
 *
 * Autor: aluno (amarqs182) - Unisinos 2026/1
 */

#ifndef OBJPARSER_H
#define OBJPARSER_H

#include <string>
#include <vector>
#include <glm/glm.hpp>

struct ObjVertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
};

struct ObjMaterial {
    std::string name;
    glm::vec3 Kd;           // Cor difusa
    glm::vec3 Ks;           // Cor especular
    glm::vec3 Ka;           // Cor ambiente
    float Ns;               // Expoente especular
    float d;                // Transparência
    std::string map_Kd;     // Caminho da textura difusa
};

struct ObjMesh {
    std::vector<ObjVertex> vertices;
    std::vector<unsigned int> indices;
    unsigned int VAO;
    unsigned int VBO;
    unsigned int EBO;
    int vertexCount;
    int indexCount;
    std::vector<ObjMaterial> materials;   // Materiais do arquivo .mtl
    std::string baseDir;                   // Diretório base do arquivo .obj
};

class ObjParser {
public:
    // Carrega um arquivo .obj e retorna um ObjMesh pronto para renderização
    static ObjMesh loadFromFile(const std::string& filename);

    // Libera os recursos de GPU de um ObjMesh
    static void cleanup(ObjMesh& mesh);

    // Retorna o caminho da textura difusa de um material específico
    static std::string getTexturePath(const ObjMesh& mesh, int materialIndex = 0);

private:
    // Converte dados carregados para formato intercalado (position + normal + texCoord)
    static ObjMesh buildMesh(
        const std::vector<glm::vec3>& positions,
        const std::vector<glm::vec3>& normals,
        const std::vector<glm::vec2>& texCoords,
        const std::vector<std::vector<int>>& faceIndices,
        const std::vector<ObjMaterial>& materials,
        const std::string& baseDir
    );

    // Carrega arquivo .mtl
    static std::vector<ObjMaterial> loadMTL(const std::string& filename);

    // Extrai o diretório base de um caminho
    static std::string getBaseDir(const std::string& filepath);
};

#endif // OBJPARSER_H
