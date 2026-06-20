/**
 * MainDiorama.cpp
 *
 * Visualizador 3D completo para a avaliação final de Computação Gráfica.
 *
 * Controles:
 *   WASD/QE     - Mover câmera
 *   Mouse       - Olhar em volta
 *   Scroll      - Zoom
 *   TAB         - Trocar objeto selecionado
 *   IJKL        - Transladar objeto selecionado
 *   U/O         - Rotacionar objeto
 *   +/-         - Escalar objeto
 *   F1/F2/F3    - Lig/des lig luzes 1/2/3
 *   F4          - Lig/des lig todas as luzes
 *   SPACE       - Ativar/desativar trajetória
 *   B           - Alternar modo Bézier
 *   1/2/3       - Adicionar/remover/limpar pontos de controle
 *   4/5         - Adicionar/remover pontos Bézier
 *   R           - Resetar trajetória
 *   UP/DOWN     - Ajustar velocidade
 *   ESC         - Sair
 *
 * Autor: aluno (amarqs182) - Unisinos 2026/1
 */

#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <fstream>
#include <sstream>
#include <algorithm>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Camera.h"
#include "ObjParser.h"

// ImGui
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

using namespace std;

// -----------------------------
// Constantes
// -----------------------------
const int MAX_LIGHTS = 3;

// -----------------------------
// Estruturas
// -----------------------------
struct Material {
    glm::vec3 color;
    float Ka;
    float Kd;
    float Ks;
    float shininess;
};

struct SceneObject {
    string id;
    string name;
    string type;
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;
    Material material;
    string texture;
};

struct SceneLight {
    string id;
    string name;
    glm::vec3 position;
    glm::vec3 color;
    float intensity;
    bool active;
};

struct RenderObject {
    GLuint VAO;
    int vertexCount;
    SceneObject config;
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;
    GLuint textureID;
    ObjMesh objMesh;      // Para modelos .obj
    bool useObjMesh;       // Se true, usa objMesh em vez de VAO/vertexCount
    string currentTexturePath;  // Caminho da textura atual
};

struct BezierTrajectory {
    bool active;
    bool useBezier;
    float speed;
    vector<glm::vec3> controlPoints;
    vector<glm::vec3> bezierPoints;
    float progress;
    size_t currentSegment;
    glm::vec3 currentPosition;
};

// -----------------------------
// Variáveis globais
// -----------------------------
GLFWwindow *Window = nullptr;
GLuint Shader_programm = 0;

int WIDTH = 1200;
int HEIGHT = 800;

float Tempo_entre_frames = 0.0f;

Camera camera(glm::vec3(0.0f, 2.0f, 8.0f));

vector<RenderObject> renderObjects;
int objeto_selecionado = 0;

SceneLight lights[MAX_LIGHTS];
int numLights = 0;

vector<BezierTrajectory> trajectories;
int objeto_trajetoria_selecionado = 0;

int Num_vertices_esfera = 0;

// -----------------------------
// Callbacks
// -----------------------------
void redimensionaCallback(GLFWwindow *window, int w, int h) {
    WIDTH = w;
    HEIGHT = h;
    glViewport(0, 0, w, h);
    camera.setProjectionParams(45.0f, (float)WIDTH / (float)HEIGHT, 0.1f, 100.0f);
}

void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    static float lastX = WIDTH / 2.0f;
    static float lastY = HEIGHT / 2.0f;
    static bool firstMouse = true;

    if (firstMouse) {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
    }

    float xoffset = (float)xpos - lastX;
    float yoffset = lastY - (float)ypos;

    lastX = (float)xpos;
    lastY = (float)ypos;

    camera.processMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    camera.processMouseScroll((float)yoffset);
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        if (key == GLFW_KEY_W) camera.processKeyboard(Camera::FORWARD, Tempo_entre_frames);
        if (key == GLFW_KEY_S) camera.processKeyboard(Camera::BACKWARD, Tempo_entre_frames);
        if (key == GLFW_KEY_A) camera.processKeyboard(Camera::LEFT, Tempo_entre_frames);
        if (key == GLFW_KEY_D) camera.processKeyboard(Camera::RIGHT_DIR, Tempo_entre_frames);
        if (key == GLFW_KEY_Q) camera.processKeyboard(Camera::DOWN, Tempo_entre_frames);
        if (key == GLFW_KEY_E) camera.processKeyboard(Camera::UP, Tempo_entre_frames);
    }

    if (action == GLFW_PRESS && !renderObjects.empty()) {
        RenderObject& obj = renderObjects[objeto_selecionado];

        if (key == GLFW_KEY_TAB) {
            objeto_selecionado = (objeto_selecionado + 1) % renderObjects.size();
            objeto_trajetoria_selecionado = objeto_selecionado;
            cout << "Objeto selecionado: " << obj.config.name << endl;
        }

        float moveSpeed = 0.2f;
        if (key == GLFW_KEY_I) obj.position.z -= moveSpeed;
        if (key == GLFW_KEY_K) obj.position.z += moveSpeed;
        if (key == GLFW_KEY_J) obj.position.x -= moveSpeed;
        if (key == GLFW_KEY_L) obj.position.x += moveSpeed;

        float rotSpeed = 5.0f;
        if (key == GLFW_KEY_U) obj.rotation.y += rotSpeed;
        if (key == GLFW_KEY_O) obj.rotation.y -= rotSpeed;

        float scaleSpeed = 0.1f;
        if (key == GLFW_KEY_EQUAL || key == GLFW_KEY_KP_ADD) {
            obj.scale += glm::vec3(scaleSpeed);
        }
        if (key == GLFW_KEY_MINUS || key == GLFW_KEY_KP_SUBTRACT) {
            obj.scale -= glm::vec3(scaleSpeed);
            if (obj.scale.x < 0.1f) obj.scale = glm::vec3(0.1f);
        }

        if (key == GLFW_KEY_F1 && numLights > 0) {
            lights[0].active = !lights[0].active;
            cout << "Luz 1: " << (lights[0].active ? "LIGADA" : "DESLIGADA") << endl;
        }
        if (key == GLFW_KEY_F2 && numLights > 1) {
            lights[1].active = !lights[1].active;
            cout << "Luz 2: " << (lights[1].active ? "LIGADA" : "DESLIGADA") << endl;
        }
        if (key == GLFW_KEY_F3 && numLights > 2) {
            lights[2].active = !lights[2].active;
            cout << "Luz 3: " << (lights[2].active ? "LIGADA" : "DESLIGADA") << endl;
        }
        if (key == GLFW_KEY_F4) {
            bool anyActive = false;
            for (int i = 0; i < numLights; i++) {
                if (lights[i].active) anyActive = true;
            }
            bool newState = !anyActive;
            for (int i = 0; i < numLights; i++) {
                lights[i].active = newState;
            }
            cout << "Todas as luzes: " << (newState ? "LIGADAS" : "DESLIGADAS") << endl;
        }

        if (key == GLFW_KEY_SPACE && objeto_trajetoria_selecionado < (int)trajectories.size()) {
            BezierTrajectory& traj = trajectories[objeto_trajetoria_selecionado];
            traj.active = !traj.active;
            cout << "Trajetoria " << (traj.active ? "ATIVADA" : "DESATIVADA") << endl;
        }

        if (key == GLFW_KEY_B && objeto_trajetoria_selecionado < (int)trajectories.size()) {
            BezierTrajectory& traj = trajectories[objeto_trajetoria_selecionado];
            traj.useBezier = !traj.useBezier;
            cout << "Modo Bezier: " << (traj.useBezier ? "ATIVADO" : "DESATIVADO") << endl;
        }

        if (key == GLFW_KEY_R && objeto_trajetoria_selecionado < (int)trajectories.size()) {
            trajectories[objeto_trajetoria_selecionado].progress = 0.0f;
            trajectories[objeto_trajetoria_selecionado].currentSegment = 0;
            cout << "Trajetoria resetada." << endl;
        }

        if (key == GLFW_KEY_UP && objeto_trajetoria_selecionado < (int)trajectories.size()) {
            trajectories[objeto_trajetoria_selecionado].speed += 0.5f;
            cout << "Velocidade: " << trajectories[objeto_trajetoria_selecionado].speed << endl;
        }

        if (key == GLFW_KEY_DOWN && objeto_trajetoria_selecionado < (int)trajectories.size()) {
            float& spd = trajectories[objeto_trajetoria_selecionado].speed;
            spd -= 0.5f;
            if (spd < 0.1f) spd = 0.1f;
            cout << "Velocidade: " << spd << endl;
        }

        if (key == GLFW_KEY_1 && objeto_trajetoria_selecionado < (int)trajectories.size()) {
            trajectories[objeto_trajetoria_selecionado].controlPoints.push_back(obj.position);
            cout << "Ponto adicionado. Total: " << trajectories[objeto_trajetoria_selecionado].controlPoints.size() << endl;
        }

        if (key == GLFW_KEY_2 && objeto_trajetoria_selecionado < (int)trajectories.size()) {
            auto& pts = trajectories[objeto_trajetoria_selecionado].controlPoints;
            if (!pts.empty()) {
                pts.pop_back();
                cout << "Ponto removido. Total: " << pts.size() << endl;
            }
        }

        if (key == GLFW_KEY_3 && objeto_trajetoria_selecionado < (int)trajectories.size()) {
            trajectories[objeto_trajetoria_selecionado].controlPoints.clear();
            cout << "Todos os pontos removidos." << endl;
        }

        if (key == GLFW_KEY_4 && objeto_trajetoria_selecionado < (int)trajectories.size()) {
            trajectories[objeto_trajetoria_selecionado].bezierPoints.push_back(obj.position);
            cout << "Ponto Bezier adicionado. Total: " << trajectories[objeto_trajetoria_selecionado].bezierPoints.size() << endl;
        }

        if (key == GLFW_KEY_5 && objeto_trajetoria_selecionado < (int)trajectories.size()) {
            auto& pts = trajectories[objeto_trajetoria_selecionado].bezierPoints;
            if (!pts.empty()) {
                pts.pop_back();
                cout << "Ponto Bezier removido. Total: " << pts.size() << endl;
            }
        }
    }
}

// -----------------------------
// OpenGL initialization
// -----------------------------
void inicializaOpenGL() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    Window = glfwCreateWindow(WIDTH, HEIGHT, "CG 2026/1 - Diorama Final", nullptr, nullptr);
    glfwMakeContextCurrent(Window);

    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    glfwSetFramebufferSizeCallback(Window, redimensionaCallback);
    glfwSetCursorPosCallback(Window, mouse_callback);
    glfwSetScrollCallback(Window, scroll_callback);
    glfwSetKeyCallback(Window, key_callback);
    glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    camera.setProjectionParams(45.0f, (float)WIDTH / (float)HEIGHT, 0.1f, 100.0f);

    // Inicializa ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(Window, true);
    ImGui_ImplOpenGL3_Init("#version 410");

    cout << "Placa de video: " << glGetString(GL_RENDERER) << endl;
    cout << "Versao do OpenGL: " << glGetString(GL_VERSION) << endl;
}

// -----------------------------
// Geometry creation
// -----------------------------
GLuint criaCubo() {
    float points[] = {
        0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
        0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
        0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
        -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f,

        0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f,
        0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f,
        -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f,
        0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f,
        -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f,
        -0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f,

        -0.5f, -0.5f, 0.5f, -1.0f, 0.0f, 0.0f,
        -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f,
        -0.5f, 0.5f, -0.5f, -1.0f, 0.0f, 0.0f,
        -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f,

        0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f,
        0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f,
        0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
        0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
        0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
        0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f,

        -0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f,
        0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f,
        0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f,
        0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f,
        -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f,
        -0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f,

        -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
        0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
        0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
        0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
        -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
        -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f
    };

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    return VAO;
}

GLuint criaEsfera() {
    vector<float> points;
    int stacks = 20;
    int sectors = 20;
    float radius = 0.5f;
    const float PI = 3.14159265359f;

    for (int i = 0; i < stacks; ++i) {
        float phi1 = PI * float(i) / stacks;
        float phi2 = PI * float(i + 1) / stacks;

        for (int j = 0; j < sectors; ++j) {
            float theta1 = 2.0f * PI * float(j) / sectors;
            float theta2 = 2.0f * PI * float(j + 1) / sectors;

            glm::vec3 v1(radius * sin(phi1) * cos(theta1), radius * cos(phi1), radius * sin(phi1) * sin(theta1));
            glm::vec3 v2(radius * sin(phi2) * cos(theta1), radius * cos(phi2), radius * sin(phi2) * sin(theta1));
            glm::vec3 v3(radius * sin(phi2) * cos(theta2), radius * cos(phi2), radius * sin(phi2) * sin(theta2));
            glm::vec3 v4(radius * sin(phi1) * cos(theta2), radius * cos(phi1), radius * sin(phi1) * sin(theta2));

            glm::vec3 n1 = glm::normalize(v1);
            glm::vec3 n2 = glm::normalize(v2);
            glm::vec3 n3 = glm::normalize(v3);
            glm::vec3 n4 = glm::normalize(v4);

            points.insert(points.end(), {v1.x, v1.y, v1.z, n1.x, n1.y, n1.z});
            points.insert(points.end(), {v2.x, v2.y, v2.z, n2.x, n2.y, n2.z});
            points.insert(points.end(), {v3.x, v3.y, v3.z, n3.x, n3.y, n3.z});

            points.insert(points.end(), {v1.x, v1.y, v1.z, n1.x, n1.y, n1.z});
            points.insert(points.end(), {v3.x, v3.y, v3.z, n3.x, n3.y, n3.z});
            points.insert(points.end(), {v4.x, v4.y, v4.z, n4.x, n4.y, n4.z});
        }
    }

    Num_vertices_esfera = points.size() / 6;

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(float), points.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    return VAO;
}

GLuint criaPiramide() {
    float points[] = {
        0.0f, 0.5f, 0.0f, 0.0f, 0.447f, 0.894f,
        -0.5f, -0.5f, 0.5f, 0.0f, 0.447f, 0.894f,
        0.5f, -0.5f, 0.5f, 0.0f, 0.447f, 0.894f,

        0.0f, 0.5f, 0.0f, 0.894f, 0.447f, 0.0f,
        0.5f, -0.5f, 0.5f, 0.894f, 0.447f, 0.0f,
        0.5f, -0.5f, -0.5f, 0.894f, 0.447f, 0.0f,

        0.0f, 0.5f, 0.0f, 0.0f, 0.447f, -0.894f,
        0.5f, -0.5f, -0.5f, 0.0f, 0.447f, -0.894f,
        -0.5f, -0.5f, -0.5f, 0.0f, 0.447f, -0.894f,

        0.0f, 0.5f, 0.0f, -0.894f, 0.447f, 0.0f,
        -0.5f, -0.5f, -0.5f, -0.894f, 0.447f, 0.0f,
        -0.5f, -0.5f, 0.5f, -0.894f, 0.447f, 0.0f,

        -0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f,
        -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f,
        0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f,

        -0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f,
        0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f,
        0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f
    };

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    return VAO;
}

// -----------------------------
// Shaders
// -----------------------------
GLuint compilaShader(const char *source, GLenum type) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        cout << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << endl;
    }

    return shader;
}

void inicializaShaders() {
    const char *vertex_shader = R"(
        #version 410

        layout(location = 0) in vec3 vertex_posicao;
        layout(location = 1) in vec3 vertex_normal;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 proj;

        out vec3 fragPos;
        out vec3 normal;

        void main()
        {
            vec4 worldPos = model * vec4(vertex_posicao, 1.0);
            fragPos = worldPos.xyz;
            normal = mat3(transpose(inverse(model))) * vertex_normal;
            gl_Position = proj * view * worldPos;
        }
    )";

    const char *fragment_shader = R"(
        #version 410

        #define MAX_LIGHTS 3

        in vec3 fragPos;
        in vec3 normal;

        out vec4 frag_colour;

        uniform vec3 viewPos;

        uniform int numLights;
        uniform vec3 lightPos[MAX_LIGHTS];
        uniform vec3 lightColor[MAX_LIGHTS];
        uniform float lightIntensity[MAX_LIGHTS];
        uniform bool lightActive[MAX_LIGHTS];

        uniform vec3 objectColor;

        uniform float Ka;
        uniform float Kd;
        uniform float Ks;
        uniform float shininess;

        uniform float Kc;
        uniform float Kl;
        uniform float Kq;

        void main()
        {
            vec3 N = normalize(normal);
            vec3 V = normalize(viewPos - fragPos);

            vec3 result = vec3(0.0);

            for (int i = 0; i < numLights; i++)
            {
                if (!lightActive[i])
                    continue;

                vec3 L = normalize(lightPos[i] - fragPos);
                vec3 R = normalize(reflect(-L, N));

                float d = length(lightPos[i] - fragPos);
                float attenuation = 1.0 / (Kc + Kl * d + Kq * (d * d));

                vec3 ambient = Ka * lightColor[i] * lightIntensity[i];

                float diff = max(dot(N, L), 0.0);
                vec3 diffuse = Kd * diff * lightColor[i] * lightIntensity[i];

                float spec = pow(max(dot(V, R), 0.0), shininess);
                vec3 specular = Ks * spec * lightColor[i] * lightIntensity[i];

                diffuse *= attenuation;
                specular *= attenuation;

                result += (ambient + diffuse) * objectColor + specular;
            }

            frag_colour = vec4(result, 1.0);
        }
    )";

    GLuint vs = compilaShader(vertex_shader, GL_VERTEX_SHADER);
    GLuint fs = compilaShader(fragment_shader, GL_FRAGMENT_SHADER);

    Shader_programm = glCreateProgram();
    glAttachShader(Shader_programm, vs);
    glAttachShader(Shader_programm, fs);
    glLinkProgram(Shader_programm);

    GLint success;
    GLchar infoLog[512];
    glGetProgramiv(Shader_programm, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(Shader_programm, 512, NULL, infoLog);
        cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << endl;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
}

// -----------------------------
// Transformations
// -----------------------------
void transformacaoGenerica(const glm::vec3& pos, const glm::vec3& rot, const glm::vec3& scl) {
    glm::mat4 transform(1.0f);
    transform = glm::translate(transform, pos);
    transform = glm::rotate(transform, glm::radians(rot.x), glm::vec3(1.0f, 0.0f, 0.0f));
    transform = glm::rotate(transform, glm::radians(rot.y), glm::vec3(0.0f, 1.0f, 0.0f));
    transform = glm::rotate(transform, glm::radians(rot.z), glm::vec3(0.0f, 0.0f, 1.0f));
    transform = glm::scale(transform, scl);

    GLuint loc = glGetUniformLocation(Shader_programm, "model");
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(transform));
}

// -----------------------------
// Camera
// -----------------------------
void atualizaCamera() {
    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 proj = camera.getProjectionMatrix();

    glUniformMatrix4fv(glGetUniformLocation(Shader_programm, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(Shader_programm, "proj"), 1, GL_FALSE, glm::value_ptr(proj));
    glUniform3fv(glGetUniformLocation(Shader_programm, "viewPos"), 1, glm::value_ptr(camera.getPosition()));
}

// -----------------------------
// Material
// -----------------------------
void defineMaterial(const Material& mat) {
    glUniform3f(glGetUniformLocation(Shader_programm, "objectColor"), mat.color.r, mat.color.g, mat.color.b);
    glUniform1f(glGetUniformLocation(Shader_programm, "Ka"), mat.Ka);
    glUniform1f(glGetUniformLocation(Shader_programm, "Kd"), mat.Kd);
    glUniform1f(glGetUniformLocation(Shader_programm, "Ks"), mat.Ks);
    glUniform1f(glGetUniformLocation(Shader_programm, "shininess"), mat.shininess);
}

// -----------------------------
// Lighting
// -----------------------------
void atualizaIluminacao() {
    glUniform1i(glGetUniformLocation(Shader_programm, "numLights"), numLights);

    for (int i = 0; i < numLights; i++) {
        string prefix = "lightPos[" + to_string(i) + "]";
        glUniform3fv(glGetUniformLocation(Shader_programm, prefix.c_str()), 1, glm::value_ptr(lights[i].position));

        prefix = "lightColor[" + to_string(i) + "]";
        glUniform3f(glGetUniformLocation(Shader_programm, prefix.c_str()), lights[i].color.r, lights[i].color.g, lights[i].color.b);

        prefix = "lightIntensity[" + to_string(i) + "]";
        glUniform1f(glGetUniformLocation(Shader_programm, prefix.c_str()), lights[i].intensity);

        prefix = "lightActive[" + to_string(i) + "]";
        glUniform1i(glGetUniformLocation(Shader_programm, prefix.c_str()), lights[i].active ? 1 : 0);
    }

    glUniform1f(glGetUniformLocation(Shader_programm, "Kc"), 1.0f);
    glUniform1f(glGetUniformLocation(Shader_programm, "Kl"), 0.09f);
    glUniform1f(glGetUniformLocation(Shader_programm, "Kq"), 0.032f);
}

// -----------------------------
// Bezier curve
// -----------------------------
glm::vec3 bezierCurve(const vector<glm::vec3>& points, float t) {
    if (points.empty()) return glm::vec3(0.0f);
    if (points.size() == 1) return points[0];
    if (points.size() == 2) return glm::mix(points[0], points[1], t);

    vector<glm::vec3> temp = points;
    int n = temp.size();

    for (int k = 1; k < n; k++) {
        for (int i = 0; i < n - k; i++) {
            temp[i] = glm::mix(temp[i], temp[i + 1], t);
        }
    }

    return temp[0];
}

// -----------------------------
// Trajectory update
// -----------------------------
void atualizaTrajetorias(float deltaTime) {
    for (size_t i = 0; i < trajectories.size() && i < renderObjects.size(); i++) {
        BezierTrajectory& traj = trajectories[i];

        if (!traj.active || traj.controlPoints.empty()) {
            renderObjects[i].position = traj.currentPosition;
            continue;
        }

        if (traj.useBezier && traj.bezierPoints.size() >= 2) {
            traj.progress += deltaTime * traj.speed * 0.2f;

            if (traj.progress >= 1.0f) {
                traj.progress = 0.0f;
                traj.currentSegment++;
                if (traj.currentSegment >= traj.bezierPoints.size() - 1) {
                    traj.currentSegment = 0;
                }
            }

            size_t startIdx = traj.currentSegment;
            vector<glm::vec3> segmentPoints;
            segmentPoints.push_back(traj.bezierPoints[startIdx]);
            if (startIdx + 2 < traj.bezierPoints.size()) {
                segmentPoints.push_back(traj.bezierPoints[startIdx + 1]);
                segmentPoints.push_back(traj.bezierPoints[startIdx + 2]);
            } else if (startIdx + 1 < traj.bezierPoints.size()) {
                segmentPoints.push_back(traj.bezierPoints[startIdx + 1]);
            }

            traj.currentPosition = bezierCurve(segmentPoints, traj.progress);
        } else {
            traj.progress += deltaTime * traj.speed * 0.2f;

            if (traj.progress >= 1.0f) {
                traj.progress = 0.0f;
                traj.currentSegment++;
                if (traj.currentSegment >= traj.controlPoints.size()) {
                    traj.currentSegment = 0;
                }
            }

            size_t startIdx = traj.currentSegment;
            size_t endIdx = (startIdx + 1) % traj.controlPoints.size();

            traj.currentPosition = glm::mix(traj.controlPoints[startIdx], traj.controlPoints[endIdx], traj.progress);
        }

        renderObjects[i].position = traj.currentPosition;
    }
}

// -----------------------------
// Texture loading
// -----------------------------
GLuint carregaTextura(const string& filename) {
    if (filename.empty()) return 0;

    GLuint textureID;
    glGenTextures(1, &textureID);

    int width, height, nrChannels;
    unsigned char *data = stbi_load(filename.c_str(), &width, &height, &nrChannels, 0);

    if (data) {
        GLenum format = GL_RGB;
        if (nrChannels == 1) format = GL_RED;
        else if (nrChannels == 3) format = GL_RGB;
        else if (nrChannels == 4) format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    } else {
        cout << "Falha ao carregar textura: " << filename << endl;
    }

    stbi_image_free(data);
    return textureID;
}

// -----------------------------
// Scene initialization
// -----------------------------
void inicializaCena() {
    // Create geometry
    GLuint vaoCubo = criaCubo();
    GLuint vaoEsfera = criaEsfera();
    GLuint vaoPiramide = criaPiramide();

    // Create objects
    SceneObject obj1;
    obj1.id = "cubo_principal";
    obj1.name = "Cubo Principal";
    obj1.type = "cube";
    obj1.position = glm::vec3(-3.0f, 0.0f, 0.0f);
    obj1.rotation = glm::vec3(0.0f);
    obj1.scale = glm::vec3(1.0f);
    obj1.material = {glm::vec3(0.8f, 0.2f, 0.1f), 0.2f, 0.7f, 1.0f, 32.0f};
    obj1.texture = "";

    SceneObject obj2;
    obj2.id = "esfera_texturizada";
    obj2.name = "Esfera Texturizada";
    obj2.type = "sphere";
    obj2.position = glm::vec3(0.0f, 0.0f, 0.0f);
    obj2.rotation = glm::vec3(0.0f);
    obj2.scale = glm::vec3(0.8f);
    obj2.material = {glm::vec3(0.2f, 0.8f, 0.2f), 0.1f, 0.7f, 0.5f, 16.0f};
    obj2.texture = "../assets/tex/pixelWall.png";

    SceneObject obj3;
    obj3.id = "piramide_azul";
    obj3.name = "Piramide Azul";
    obj3.type = "pyramid";
    obj3.position = glm::vec3(3.0f, 0.0f, 0.0f);
    obj3.rotation = glm::vec3(0.0f, 45.0f, 0.0f);
    obj3.scale = glm::vec3(1.0f, 1.5f, 1.0f);
    obj3.material = {glm::vec3(0.2f, 0.2f, 1.0f), 0.1f, 0.7f, 0.8f, 64.0f};
    obj3.texture = "";

    SceneObject obj4;
    obj4.id = "chao";
    obj4.name = "Chao";
    obj4.type = "cube";
    obj4.position = glm::vec3(0.0f, -1.5f, 0.0f);
    obj4.rotation = glm::vec3(0.0f);
    obj4.scale = glm::vec3(15.0f, 0.1f, 15.0f);
    obj4.material = {glm::vec3(0.4f, 0.4f, 0.4f), 0.3f, 0.6f, 0.1f, 8.0f};
    obj4.texture = "";

    SceneObject obj5;
    obj5.id = "suzanne";
    obj5.name = "Suzanne (OBJ)";
    obj5.type = "obj";
    obj5.position = glm::vec3(0.0f, 1.0f, 3.0f);
    obj5.rotation = glm::vec3(0.0f);
    obj5.scale = glm::vec3(1.0f);
    obj5.material = {glm::vec3(0.8f, 0.5f, 0.2f), 0.2f, 0.7f, 0.8f, 32.0f};
    obj5.texture = "../assets/Modelos3D/Suzanne.obj";

    vector<SceneObject*> objects = {&obj1, &obj2, &obj3, &obj4, &obj5};

    for (auto* obj : objects) {
        RenderObject renderObj;
        renderObj.config = *obj;
        renderObj.position = obj->position;
        renderObj.rotation = obj->rotation;
        renderObj.scale = obj->scale;
        renderObj.textureID = 0;
        renderObj.useObjMesh = false;

        if (obj->type == "cube") {
            renderObj.VAO = vaoCubo;
            renderObj.vertexCount = 36;
        } else if (obj->type == "sphere") {
            renderObj.VAO = vaoEsfera;
            renderObj.vertexCount = Num_vertices_esfera;
        } else if (obj->type == "pyramid") {
            renderObj.VAO = vaoPiramide;
            renderObj.vertexCount = 18;
        } else if (obj->type == "obj") {
            // Carrega modelo .obj com materiais
            renderObj.objMesh = ObjParser::loadFromFile(obj->texture);
            renderObj.useObjMesh = true;

            // Tenta carregar textura automaticamente do .mtl
            string texPath = ObjParser::getTexturePath(renderObj.objMesh);
            if (!texPath.empty()) {
                renderObj.textureID = carregaTextura(texPath);
                renderObj.currentTexturePath = texPath;
                cout << "Textura automatica carregada: " << texPath << endl;
            } else {
                renderObj.textureID = 0;
                renderObj.currentTexturePath = "";
            }
        } else {
            renderObj.VAO = vaoCubo;
            renderObj.vertexCount = 36;
        }

        if (!obj->texture.empty() && obj->type != "obj") {
            renderObj.textureID = carregaTextura(obj->texture);
        }

        renderObjects.push_back(renderObj);

        BezierTrajectory traj;
        traj.active = false;
        traj.useBezier = false;
        traj.speed = 1.5f;
        traj.progress = 0.0f;
        traj.currentSegment = 0;
        traj.currentPosition = obj->position;
        trajectories.push_back(traj);
    }

    // Setup lights
    numLights = 3;
    lights[0] = {"luz_principal", "Key Light", glm::vec3(5.0f, 5.0f, 5.0f), glm::vec3(1.0f), 1.0f, true};
    lights[1] = {"luz_secundaria", "Fill Light", glm::vec3(-3.0f, 3.0f, 2.0f), glm::vec3(0.8f, 0.8f, 1.0f), 0.6f, true};
    lights[2] = {"luz_traseira", "Back Light", glm::vec3(0.0f, 3.0f, -5.0f), glm::vec3(1.0f, 0.9f, 0.8f), 0.4f, true};

    // Add initial trajectory points for cube
    trajectories[0].controlPoints.push_back(glm::vec3(-3.0f, 0.0f, 0.0f));
    trajectories[0].controlPoints.push_back(glm::vec3(-3.0f, 2.0f, 0.0f));
    trajectories[0].controlPoints.push_back(glm::vec3(-1.0f, 2.0f, 0.0f));
    trajectories[0].controlPoints.push_back(glm::vec3(-1.0f, 0.0f, 0.0f));

    cout << "\n=== Controles ===" << endl;
    cout << "WASD/QE - Mover camera" << endl;
    cout << "Mouse - Olhar em volta" << endl;
    cout << "TAB - Trocar objeto selecionado" << endl;
    cout << "IJKL - Transladar objeto" << endl;
    cout << "U/O - Rotacionar objeto" << endl;
    cout << "+/- - Escalar objeto" << endl;
    cout << "F1/F2/F3 - Lig/deslig luzes" << endl;
    cout << "F4 - Lig/deslig todas as luzes" << endl;
    cout << "SPACE - Ativar/desativar trajetoria" << endl;
    cout << "B - Alternar modo Bezier" << endl;
    cout << "1/2/3 - Adicionar/remover/limpar pontos" << endl;
    cout << "4/5 - Adicionar/remover pontos Bezier" << endl;
    cout << "R - Resetar trajetoria" << endl;
    cout << "UP/DOWN - Ajustar velocidade" << endl;
    cout << "ESC - Sair" << endl;
    cout << "==================\n" << endl;
}

// -----------------------------
// Draw control points
// -----------------------------
void desenhaPontosControle() {
    for (size_t i = 0; i < trajectories.size(); i++) {
        const BezierTrajectory& traj = trajectories[i];

        for (size_t j = 0; j < traj.controlPoints.size(); j++) {
            Material mat;
            mat.color = glm::vec3(0.0f, 1.0f, 0.0f);
            mat.Ka = 0.2f;
            mat.Kd = 0.8f;
            mat.Ks = 0.5f;
            mat.shininess = 16.0f;

            defineMaterial(mat);

            float escala = 0.1f;
            if (j == traj.currentSegment && i == (size_t)objeto_trajetoria_selecionado)
                escala = 0.15f;

            transformacaoGenerica(traj.controlPoints[j], glm::vec3(0.0f), glm::vec3(escala));
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        for (size_t j = 0; j < traj.bezierPoints.size(); j++) {
            Material mat;
            mat.color = glm::vec3(1.0f, 0.0f, 1.0f);
            mat.Ka = 0.2f;
            mat.Kd = 0.8f;
            mat.Ks = 0.5f;
            mat.shininess = 16.0f;

            defineMaterial(mat);

            transformacaoGenerica(traj.bezierPoints[j], glm::vec3(0.0f), glm::vec3(0.12f));
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
    }
}

// -----------------------------
// Render loop
// -----------------------------
void inicializaRenderizacao() {
    float tempo_anterior = (float)glfwGetTime();

    glEnable(GL_DEPTH_TEST);

    while (!glfwWindowShouldClose(Window)) {
        float tempo_atual = (float)glfwGetTime();
        Tempo_entre_frames = tempo_atual - tempo_anterior;
        tempo_anterior = tempo_atual;

        // Inicia novo frame do ImGui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        atualizaTrajetorias(Tempo_entre_frames);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(Shader_programm);
        atualizaCamera();
        atualizaIluminacao();

        for (size_t i = 0; i < renderObjects.size(); i++) {
            RenderObject& obj = renderObjects[i];

            defineMaterial(obj.config.material);
            transformacaoGenerica(obj.position, obj.rotation, obj.scale);

            if (obj.textureID != 0) {
                glBindTexture(GL_TEXTURE_2D, obj.textureID);
            }

            if (obj.useObjMesh) {
                // Renderiza modelo .obj
                glBindVertexArray(obj.objMesh.VAO);
                glDrawElements(GL_TRIANGLES, obj.objMesh.indexCount, GL_UNSIGNED_INT, 0);
            } else {
                // Renderiza geometria procedural
                glBindVertexArray(obj.VAO);
                glDrawArrays(GL_TRIANGLES, 0, obj.vertexCount);
            }
        }

        desenhaPontosControle();

        // ImGui Panel
        {
            ImGui::Begin("CG 2026/1 - Diorama");

            // Objeto selecionado
            if (objeto_selecionado < (int)renderObjects.size()) {
                RenderObject& sel = renderObjects[objeto_selecionado];
                ImGui::Text("Objeto: %s", sel.config.name.c_str());
                ImGui::Separator();

                // Transformações
                ImGui::Text("Transformacoes:");
                ImGui::DragFloat3("Posicao", &sel.position.x, 0.1f);
                ImGui::DragFloat3("Rotacao", &sel.rotation.x, 1.0f, 0.0f, 360.0f);
                ImGui::DragFloat3("Escala", &sel.scale.x, 0.05f, 0.1f, 10.0f);

                ImGui::Separator();

                // Material
                ImGui::Text("Material:");
                ImGui::ColorEdit3("Cor", &sel.config.material.color.x);
                ImGui::SliderFloat("Ka", &sel.config.material.Ka, 0.0f, 1.0f);
                ImGui::SliderFloat("Kd", &sel.config.material.Kd, 0.0f, 1.0f);
                ImGui::SliderFloat("Ks", &sel.config.material.Ks, 0.0f, 1.0f);
                ImGui::SliderFloat("Shininess", &sel.config.material.shininess, 1.0f, 256.0f);

                ImGui::Separator();

                // Textura
                ImGui::Text("Textura:");
                if (sel.textureID != 0) {
                    ImGui::Text("Atual: %s", sel.currentTexturePath.c_str());
                    if (ImGui::Button("Remover Textura")) {
                        sel.textureID = 0;
                        sel.currentTexturePath = "";
                    }
                } else {
                    ImGui::Text("Nenhuma textura");
                }

                if (ImGui::TreeNode("Mudar Textura")) {
                    vector<pair<string, string>> textures = {
                        {"Nenhuma", ""},
                        {"Pixel Wall", "../assets/tex/pixelWall.png"},
                        {"Suzanne", "../assets/Modelos3D/Suzanne.png"},
                        {"Suzanne UV", "../assets/Modelos3D/SuzanneUV.png"}
                    };

                    for (const auto& tex : textures) {
                        if (ImGui::Selectable(tex.first.c_str())) {
                            if (tex.second.empty()) {
                                sel.textureID = 0;
                                sel.currentTexturePath = "";
                            } else {
                                GLuint newTex = carregaTextura(tex.second);
                                if (newTex != 0) {
                                    sel.textureID = newTex;
                                    sel.currentTexturePath = tex.second;
                                }
                            }
                        }
                    }

                    if (sel.useObjMesh && !sel.objMesh.materials.empty()) {
                        ImGui::Separator();
                        ImGui::Text("Texturas .mtl:");
                        for (size_t i = 0; i < sel.objMesh.materials.size(); i++) {
                            const ObjMaterial& mat = sel.objMesh.materials[i];
                            if (!mat.map_Kd.empty()) {
                                string fullPath = sel.objMesh.baseDir + mat.map_Kd;
                                string label = mat.name + "##" + to_string(i);
                                if (ImGui::Selectable(label.c_str())) {
                                    GLuint newTex = carregaTextura(fullPath);
                                    if (newTex != 0) {
                                        sel.textureID = newTex;
                                        sel.currentTexturePath = fullPath;
                                    }
                                }
                            }
                        }
                    }

                    ImGui::TreePop();
                }
            }

            ImGui::Separator();

            // Luzes
            ImGui::Text("Luzes:");
            static float lightColors[MAX_LIGHTS][3] = {
                {1.0f, 1.0f, 1.0f},
                {0.8f, 0.8f, 1.0f},
                {1.0f, 0.9f, 0.8f}
            };
            for (int i = 0; i < numLights; i++) {
                string label = "Luz " + to_string(i + 1) + "##" + to_string(i);
                if (ImGui::TreeNode(label.c_str())) {
                    ImGui::Checkbox("Ativa", &lights[i].active);
                    ImGui::DragFloat3("Posicao", &lights[i].position.x, 0.1f);
                    ImGui::ColorEdit3("Cor", lightColors[i]);
                    lights[i].color = glm::vec3(lightColors[i][0], lightColors[i][1], lightColors[i][2]);
                    ImGui::SliderFloat("Intensidade", &lights[i].intensity, 0.0f, 2.0f);
                    ImGui::TreePop();
                }
            }

            ImGui::Separator();

            // Trajetória
            if (objeto_trajetoria_selecionado < (int)trajectories.size()) {
                BezierTrajectory& traj = trajectories[objeto_trajetoria_selecionado];
                ImGui::Text("Trajetoria:");
                ImGui::Checkbox("Ativa", &traj.active);
                ImGui::Checkbox("Modo Bezier", &traj.useBezier);
                ImGui::SliderFloat("Velocidade", &traj.speed, 0.1f, 5.0f);
                ImGui::Text("Pontos: %d", (int)traj.controlPoints.size());
                ImGui::Text("Bezier: %d", (int)traj.bezierPoints.size());
            }

            ImGui::Separator();
            ImGui::Text("Camera: %.1f, %.1f, %.1f", camera.getPosition().x, camera.getPosition().y, camera.getPosition().z);

            ImGui::End();
        }

        // Renderiza ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(Window);
        glfwPollEvents();
    }

    // Cleanup ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
}

// -----------------------------
// Main
// -----------------------------
int main() {
    inicializaOpenGL();
    inicializaShaders();
    inicializaCena();
    inicializaRenderizacao();
    return 0;
}
