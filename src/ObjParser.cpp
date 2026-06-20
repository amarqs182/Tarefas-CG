/**
 * ObjParser.cpp
 *
 * Implementação do parser de .obj com suporte a .mtl.
 *
 * Autor: aluno (amarqs182) - Unisinos 2026/1
 */

#include "ObjParser.h"
#include <glad/glad.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <tuple>

using namespace std;

// Extrai o diretório base de um caminho
string ObjParser::getBaseDir(const string& filepath) {
    size_t lastSlash = filepath.find_last_of("/\\");
    if (lastSlash != string::npos) {
        return filepath.substr(0, lastSlash + 1);
    }
    return "./";
}

// Carrega arquivo .mtl
vector<ObjMaterial> ObjParser::loadMTL(const string& filename) {
    vector<ObjMaterial> materials;
    ObjMaterial currentMaterial;

    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Aviso: Nao foi possivel abrir arquivo .mtl: " << filename << endl;
        return materials;
    }

    string line;
    while (getline(file, line)) {
        istringstream iss(line);
        string prefix;
        iss >> prefix;

        if (prefix == "newmtl") {
            // Novo material - salva o anterior se existir
            if (!currentMaterial.name.empty()) {
                materials.push_back(currentMaterial);
            }
            currentMaterial = ObjMaterial();
            iss >> currentMaterial.name;
        }
        else if (prefix == "Kd") {
            // Cor difusa
            iss >> currentMaterial.Kd.x >> currentMaterial.Kd.y >> currentMaterial.Kd.z;
        }
        else if (prefix == "Ks") {
            // Cor especular
            iss >> currentMaterial.Ks.x >> currentMaterial.Ks.y >> currentMaterial.Ks.z;
        }
        else if (prefix == "Ka") {
            // Cor ambiente
            iss >> currentMaterial.Ka.x >> currentMaterial.Ka.y >> currentMaterial.Ka.z;
        }
        else if (prefix == "Ns") {
            // Expoente especular
            iss >> currentMaterial.Ns;
        }
        else if (prefix == "d") {
            // Transparência
            iss >> currentMaterial.d;
        }
        else if (prefix == "map_Kd") {
            // Caminho da textura difusa
            string path;
            iss >> path;
            // Remove aspas se existirem
            if (!path.empty() && path.front() == '"') {
                path = path.substr(1, path.size() - 2);
            }
            currentMaterial.map_Kd = path;
        }
    }

    // Salva o último material
    if (!currentMaterial.name.empty()) {
        materials.push_back(currentMaterial);
    }

    file.close();

    cout << "Material carregado: " << filename << endl;
    cout << "  Materiais encontrados: " << materials.size() << endl;

    return materials;
}

ObjMesh ObjParser::loadFromFile(const string& filename) {
    ObjMesh mesh = {};

    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Erro ao abrir arquivo .obj: " << filename << endl;
        return mesh;
    }

    string baseDir = getBaseDir(filename);
    mesh.baseDir = baseDir;

    vector<glm::vec3> positions;
    vector<glm::vec3> normals;
    vector<glm::vec2> texCoords;
    vector<vector<int>> faceIndices;
    vector<ObjMaterial> materials;

    string line;
    while (getline(file, line)) {
        istringstream iss(line);
        string prefix;
        iss >> prefix;

        if (prefix == "v") {
            glm::vec3 pos;
            iss >> pos.x >> pos.y >> pos.z;
            positions.push_back(pos);
        }
        else if (prefix == "vn") {
            glm::vec3 norm;
            iss >> norm.x >> norm.y >> norm.z;
            normals.push_back(norm);
        }
        else if (prefix == "vt") {
            glm::vec2 tc;
            iss >> tc.x >> tc.y;
            texCoords.push_back(tc);
        }
        else if (prefix == "mtllib") {
            // Carrega arquivo .mtl
            string mtlFile;
            iss >> mtlFile;
            materials = loadMTL(baseDir + mtlFile);
        }
        else if (prefix == "f") {
            vector<int> faceV, faceVT, faceVN;
            string vertex;

            while (iss >> vertex) {
                int v = 0, vt = 0, vn = 0;

                size_t slash1 = vertex.find('/');
                size_t slash2 = vertex.rfind('/');

                if (slash1 == string::npos) {
                    v = stoi(vertex);
                }
                else if (slash1 == slash2) {
                    v = stoi(vertex.substr(0, slash1));
                    vt = stoi(vertex.substr(slash1 + 1));
                }
                else if (slash2 - slash1 == 1) {
                    v = stoi(vertex.substr(0, slash1));
                    vn = stoi(vertex.substr(slash2 + 1));
                }
                else {
                    v = stoi(vertex.substr(0, slash1));
                    string mid = vertex.substr(slash1 + 1, slash2 - slash1 - 1);
                    vt = mid.empty() ? 0 : stoi(mid);
                    vn = stoi(vertex.substr(slash2 + 1));
                }

                faceV.push_back(v);
                faceVT.push_back(vt);
                faceVN.push_back(vn);
            }

            // Triangula faces com mais de 3 vértices
            for (size_t i = 1; i + 1 < faceV.size(); i++) {
                vector<int> triV = {faceV[0], faceV[i], faceV[i + 1]};
                vector<int> triVT = {faceVT[0], faceVT[i], faceVT[i + 1]};
                vector<int> triVN = {faceVN[0], faceVN[i], faceVN[i + 1]};

                for (int j = 0; j < 3; j++) {
                    faceIndices.push_back({triV[j], triVT[j], triVN[j]});
                }
            }
        }
    }

    file.close();

    // Constrói a mesh
    mesh = buildMesh(positions, normals, texCoords, faceIndices, materials, baseDir);

    cout << "Modelo carregado: " << filename << endl;
    cout << "  Vertices: " << mesh.vertexCount << endl;
    cout << "  Indices: " << mesh.indexCount << endl;
    cout << "  Materiais: " << mesh.materials.size() << endl;

    // Mostra informações dos materiais
    for (size_t i = 0; i < mesh.materials.size(); i++) {
        const ObjMaterial& mat = mesh.materials[i];
        cout << "  Material " << i << ": " << mat.name << endl;
        if (!mat.map_Kd.empty()) {
            cout << "    Textura: " << mat.map_Kd << endl;
        }
    }

    return mesh;
}

ObjMesh ObjParser::buildMesh(
    const vector<glm::vec3>& positions,
    const vector<glm::vec3>& normals,
    const vector<glm::vec2>& texCoords,
    const vector<vector<int>>& faceIndices,
    const vector<ObjMaterial>& materials,
    const string& baseDir
) {
    ObjMesh mesh = {};

    // Mapeamento para eliminar vértices duplicados
    map<tuple<int, int, int>, unsigned int> uniqueVertices;
    vector<ObjVertex> vertexData;
    vector<unsigned int> indices;

    for (const auto& face : faceIndices) {
        int vi = face[0] - 1;
        int vti = face[1] - 1;
        int vni = face[2] - 1;

        auto key = make_tuple(vi, vti, vni);

        if (uniqueVertices.find(key) != uniqueVertices.end()) {
            indices.push_back(uniqueVertices[key]);
        }
        else {
            ObjVertex vertex;

            if (vi >= 0 && vi < (int)positions.size()) {
                vertex.position = positions[vi];
            }

            if (vni >= 0 && vni < (int)normals.size()) {
                vertex.normal = normals[vni];
            }
            else {
                vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
            }

            if (vti >= 0 && vti < (int)texCoords.size()) {
                vertex.texCoord = texCoords[vti];
            }

            unsigned int index = vertexData.size();
            uniqueVertices[key] = index;
            vertexData.push_back(vertex);
            indices.push_back(index);
        }
    }

    mesh.vertices = vertexData;
    mesh.indices = indices;
    mesh.vertexCount = vertexData.size();
    mesh.indexCount = indices.size();
    mesh.materials = materials;
    mesh.baseDir = baseDir;

    // Cria buffers de GPU
    glGenVertexArrays(1, &mesh.VAO);
    glGenBuffers(1, &mesh.VBO);
    glGenBuffers(1, &mesh.EBO);

    glBindVertexArray(mesh.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
    glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(ObjVertex), vertexData.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Atributo 0: Posição
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ObjVertex), (void*)0);
    glEnableVertexAttribArray(0);

    // Atributo 1: Normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(ObjVertex), (void*)offsetof(ObjVertex, normal));
    glEnableVertexAttribArray(1);

    // Atributo 2: Coordenada de textura
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(ObjVertex), (void*)offsetof(ObjVertex, texCoord));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    return mesh;
}

string ObjParser::getTexturePath(const ObjMesh& mesh, int materialIndex) {
    if (materialIndex >= 0 && materialIndex < (int)mesh.materials.size()) {
        const ObjMaterial& mat = mesh.materials[materialIndex];
        if (!mat.map_Kd.empty()) {
            return mesh.baseDir + mat.map_Kd;
        }
    }
    return "";
}

void ObjParser::cleanup(ObjMesh& mesh) {
    if (mesh.VAO) glDeleteVertexArrays(1, &mesh.VAO);
    if (mesh.VBO) glDeleteBuffers(1, &mesh.VBO);
    if (mesh.EBO) glDeleteBuffers(1, &mesh.EBO);
    mesh = {};
}
