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
#include "Trajectory.h"

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

// Trajetoria agora usa a classe Trajectory (definida em include/Trajectory.h),
// que ja encapsula interpolacao linear + bezier cubico via enum Interpolation.
// Ver refactor/use-trajectory-class no historico do git.
using Traj = Trajectory;

std::vector<Traj> trajectories;

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

int objeto_trajetoria_selecionado = 0;

int Num_vertices_esfera = 0;

bool cursorLivre = false;

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
    if (cursorLivre) return;

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
    if (cursorLivre) return;
    camera.processMouseScroll((float)yoffset);
}

// Processa teclas continuas (WASD/IJKL/UO/+/-) por FRAME via glfwGetKey,
// nao via REPEAT-key_callback. REPEAT conflitava com sliders do ImGui
// (teclas W/A/S/D/I servem para navegar sliders por convenção do ImGui).
void processaInputContinuo() {
    if (cursorLivre || renderObjects.empty()) return;

    // Câmera
    if (glfwGetKey(Window, GLFW_KEY_W) == GLFW_PRESS) camera.processKeyboard(Camera::FORWARD,  Tempo_entre_frames);
    if (glfwGetKey(Window, GLFW_KEY_S) == GLFW_PRESS) camera.processKeyboard(Camera::BACKWARD, Tempo_entre_frames);
    if (glfwGetKey(Window, GLFW_KEY_A) == GLFW_PRESS) camera.processKeyboard(Camera::LEFT,      Tempo_entre_frames);
    if (glfwGetKey(Window, GLFW_KEY_D) == GLFW_PRESS) camera.processKeyboard(Camera::RIGHT_DIR, Tempo_entre_frames);
    if (glfwGetKey(Window, GLFW_KEY_Q) == GLFW_PRESS) camera.processKeyboard(Camera::DOWN,      Tempo_entre_frames);
    if (glfwGetKey(Window, GLFW_KEY_E) == GLFW_PRESS) camera.processKeyboard(Camera::UP,        Tempo_entre_frames);

    // Objeto selecionado
    RenderObject& obj = renderObjects[objeto_selecionado];

    const float moveSpeed = 0.2f;
    if (glfwGetKey(Window, GLFW_KEY_I) == GLFW_PRESS) obj.position.z -= moveSpeed;
    if (glfwGetKey(Window, GLFW_KEY_K) == GLFW_PRESS) obj.position.z += moveSpeed;
    if (glfwGetKey(Window, GLFW_KEY_J) == GLFW_PRESS) obj.position.x -= moveSpeed;
    if (glfwGetKey(Window, GLFW_KEY_L) == GLFW_PRESS) obj.position.x += moveSpeed;

    const float rotSpeed = 5.0f;
    if (glfwGetKey(Window, GLFW_KEY_U) == GLFW_PRESS) obj.rotation.y += rotSpeed;
    if (glfwGetKey(Window, GLFW_KEY_O) == GLFW_PRESS) obj.rotation.y -= rotSpeed;

    const float scaleSpeed = 0.1f;
    if (glfwGetKey(Window, GLFW_KEY_EQUAL)  == GLFW_PRESS || glfwGetKey(Window, GLFW_KEY_KP_ADD) == GLFW_PRESS) {
        obj.scale += glm::vec3(scaleSpeed);
    }
    if (glfwGetKey(Window, GLFW_KEY_MINUS) == GLFW_PRESS || glfwGetKey(Window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS) {
        obj.scale -= glm::vec3(scaleSpeed);
        if (obj.scale.x < 0.1f) obj.scale = glm::vec3(0.1f);
    }
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (key == GLFW_KEY_GRAVE_ACCENT && action == GLFW_PRESS) {
        cursorLivre = !cursorLivre;
        if (cursorLivre) {
            glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }

    // Os comandos REPEAT foram REMOVIDOS deste callback porque conflitavam
    // com sliders do ImGui (W/A/S/D/I/J/K/L/U/O/+/- sao teclas de navegar
    // sliders por padrao). Agora eles sao processados em processaInputContinuo()
    // dentro do loop principal via glfwGetKey(), que nao conflita com ImGui.

    if (action == GLFW_PRESS && !renderObjects.empty() && !cursorLivre) {

        if (key == GLFW_KEY_F && mods == GLFW_MOD_SHIFT && action == GLFW_PRESS) {
            objeto_selecionado = (objeto_selecionado + 1) % renderObjects.size();
            objeto_trajetoria_selecionado = objeto_selecionado;
            cout << "Objeto selecionado: " << renderObjects[objeto_selecionado].config.name << endl;
        }

        RenderObject& obj = renderObjects[objeto_selecionado];

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
            Traj& traj = trajectories[objeto_trajetoria_selecionado];
            traj.setActive(!traj.isActive());
            cout << "Trajetoria " << (traj.isActive() ? "ATIVADA" : "DESATIVADA") << endl;
        }

        if (key == GLFW_KEY_B && objeto_trajetoria_selecionado < (int)trajectories.size()) {
            Traj& traj = trajectories[objeto_trajetoria_selecionado];
            // Toggle entre LINEAR e BEZIER via API da classe Trajectory
            Trajectory::Interpolation novoModo =
                (traj.getInterpolation() == Trajectory::Interpolation::LINEAR)
                ? Trajectory::Interpolation::BEZIER
                : Trajectory::Interpolation::LINEAR;
            traj.setInterpolation(novoModo);
            cout << "Modo interpolacao: "
                 << (novoModo == Trajectory::Interpolation::BEZIER ? "BEZIER" : "LINEAR")
                 << endl;
        }

        if (key == GLFW_KEY_R && objeto_trajetoria_selecionado < (int)trajectories.size()) {
            trajectories[objeto_trajetoria_selecionado].reset();
            cout << "Trajetoria resetada." << endl;
        }

        if (key == GLFW_KEY_UP && objeto_trajetoria_selecionado < (int)trajectories.size()) {
            Traj& traj = trajectories[objeto_trajetoria_selecionado];
            traj.setSpeed(traj.getSpeed() + 0.5f);
            cout << "Velocidade: " << traj.getSpeed() << endl;
        }

        if (key == GLFW_KEY_DOWN && objeto_trajetoria_selecionado < (int)trajectories.size()) {
            Traj& traj = trajectories[objeto_trajetoria_selecionado];
            float spd = traj.getSpeed() - 0.5f;
            if (spd < 0.1f) spd = 0.1f;
            traj.setSpeed(spd);
            cout << "Velocidade: " << spd << endl;
        }

        if (key == GLFW_KEY_1 && objeto_trajetoria_selecionado < (int)trajectories.size()) {
            trajectories[objeto_trajetoria_selecionado].addPoint(obj.position);
            cout << "Ponto adicionado. Total: " << trajectories[objeto_trajetoria_selecionado].getPointCount() << endl;
        }

        if (key == GLFW_KEY_2 && objeto_trajetoria_selecionado < (int)trajectories.size()) {
            Traj& traj = trajectories[objeto_trajetoria_selecionado];
            size_t n = traj.getPointCount();
            if (n > 0) {
                traj.removePoint(n - 1);
                cout << "Ponto removido. Total: " << traj.getPointCount() << endl;
            }
        }

        if (key == GLFW_KEY_3 && objeto_trajetoria_selecionado < (int)trajectories.size()) {
            trajectories[objeto_trajetoria_selecionado].clearPoints();
            cout << "Todos os pontos removidos." << endl;
        }

        if (key == GLFW_KEY_4 && objeto_trajetoria_selecionado < (int)trajectories.size()) {
            trajectories[objeto_trajetoria_selecionado].addBezierPoint(obj.position);
            cout << "Ponto Bezier adicionado. Total: " << trajectories[objeto_trajetoria_selecionado].getBezierPointCount() << endl;
        }

        if (key == GLFW_KEY_5 && objeto_trajetoria_selecionado < (int)trajectories.size()) {
            Traj& traj = trajectories[objeto_trajetoria_selecionado];
            size_t n = traj.getBezierPointCount();
            if (n > 0) {
                traj.removeBezierPoint(n - 1);
                cout << "Ponto Bezier removido. Total: " << traj.getBezierPointCount() << endl;
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
    io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;
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
    // Formato: pos(3) + normal(3) + uv(2) = 8 floats por vertice
    float points[] = {
        // Frente (z+)
        0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
        0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,
        -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
        -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,

        // Tras (z-)
        0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f,
        0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f,
        0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f,
        -0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,

        // Esquerda (x-)
        -0.5f, -0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
        -0.5f, 0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
        -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,

        // Direita (x+)
        0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
        0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
        0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
        0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
        0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,

        // Baixo (y-)
        -0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f,
        0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f,
        0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f,

        // Cima (y+)
        -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
        0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
        0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
        0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
        -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f
    };

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    return VAO;
}

GLuint criaEsfera() {
    // Formato: pos(3) + normal(3) + uv(2) = 8 floats por vertice
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

            // UV: u = theta/(2*PI), v = phi/PI
            float u1 = theta1 / (2.0f * PI);
            float u2 = theta2 / (2.0f * PI);
            float v_1 = phi1 / PI;
            float v_2 = phi2 / PI;

            points.insert(points.end(), {v1.x, v1.y, v1.z, n1.x, n1.y, n1.z, u1, v_1});
            points.insert(points.end(), {v2.x, v2.y, v2.z, n2.x, n2.y, n2.z, u1, v_2});
            points.insert(points.end(), {v3.x, v3.y, v3.z, n3.x, n3.y, n3.z, u2, v_2});

            points.insert(points.end(), {v1.x, v1.y, v1.z, n1.x, n1.y, n1.z, u1, v_1});
            points.insert(points.end(), {v3.x, v3.y, v3.z, n3.x, n3.y, n3.z, u2, v_2});
            points.insert(points.end(), {v4.x, v4.y, v4.z, n4.x, n4.y, n4.z, u2, v_1});
        }
    }

    Num_vertices_esfera = points.size() / 8;

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(float), points.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    return VAO;
}

GLuint criaPiramide() {
    // Formato: pos(3) + normal(3) + uv(2) = 8 floats por vertice
    float points[] = {
        // Frente
        0.0f, 0.5f, 0.0f, 0.0f, 0.447f, 0.894f, 0.5f, 1.0f,
        -0.5f, -0.5f, 0.5f, 0.0f, 0.447f, 0.894f, 0.0f, 0.0f,
        0.5f, -0.5f, 0.5f, 0.0f, 0.447f, 0.894f, 1.0f, 0.0f,

        // Direita
        0.0f, 0.5f, 0.0f, 0.894f, 0.447f, 0.0f, 0.5f, 1.0f,
        0.5f, -0.5f, 0.5f, 0.894f, 0.447f, 0.0f, 0.0f, 0.0f,
        0.5f, -0.5f, -0.5f, 0.894f, 0.447f, 0.0f, 1.0f, 0.0f,

        // Tras
        0.0f, 0.5f, 0.0f, 0.0f, 0.447f, -0.894f, 0.5f, 1.0f,
        0.5f, -0.5f, -0.5f, 0.0f, 0.447f, -0.894f, 0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f, 0.0f, 0.447f, -0.894f, 1.0f, 0.0f,

        // Esquerda
        0.0f, 0.5f, 0.0f, -0.894f, 0.447f, 0.0f, 0.5f, 1.0f,
        -0.5f, -0.5f, -0.5f, -0.894f, 0.447f, 0.0f, 0.0f, 0.0f,
        -0.5f, -0.5f, 0.5f, -0.894f, 0.447f, 0.0f, 1.0f, 0.0f,

        // Base 1
        -0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f,
        0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f,

        // Base 2
        -0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f,
        0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f,
        0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f
    };

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

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
        layout(location = 2) in vec2 vertex_texCoord;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 proj;

        out vec3 fragPos;
        out vec3 normal;
        out vec2 vTexCoord;

        void main()
        {
            vec4 worldPos = model * vec4(vertex_posicao, 1.0);
            fragPos = worldPos.xyz;
            normal = mat3(transpose(inverse(model))) * vertex_normal;
            vTexCoord = vertex_texCoord;
            gl_Position = proj * view * worldPos;
        }
    )";

    const char *fragment_shader = R"(
        #version 410

        #define MAX_LIGHTS 3

        in vec3 fragPos;
        in vec3 normal;
        in vec2 vTexCoord;

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

        uniform sampler2D uTexture;
        uniform bool useTexture;

        void main()
        {
            vec3 N = normalize(normal);
            vec3 V = normalize(viewPos - fragPos);

            // Cor base: textura quando disponivel, senao cor do material
            vec3 baseColor = useTexture ? texture(uTexture, vTexCoord).rgb : objectColor;

            // Ambient GLOBAL: soma de TODAS as luzes (ativas ou nao).
            // Luz desligada em "luz ambientes" era o bug #4 — agora ambient
            // e sempre contabilizado por luz (mesmo se so do furo de Phong).
            vec3 ambientTotal = vec3(0.0);
            for (int i = 0; i < numLights; i++) {
                ambientTotal += Ka * lightColor[i] * lightIntensity[i];
            }
            vec3 ambient = ambientTotal * baseColor;

            vec3 diffuseSum = vec3(0.0);
            vec3 specularSum = vec3(0.0);

            for (int i = 0; i < numLights; i++) {
                if (!lightActive[i])
                    continue;

                vec3 L = normalize(lightPos[i] - fragPos);
                vec3 R = normalize(reflect(-L, N));

                float d = length(lightPos[i] - fragPos);
                float attenuation = 1.0 / (Kc + Kl * d + Kq * (d * d));

                float diff = max(dot(N, L), 0.0);
                vec3 diffuse = Kd * diff * lightColor[i] * lightIntensity[i];

                float spec = pow(max(dot(V, R), 0.0), shininess);
                vec3 specular = Ks * spec * lightColor[i] * lightIntensity[i];

                diffuse *= attenuation;
                specular *= attenuation;

                diffuseSum  += diffuse  * baseColor;
                specularSum += specular;
            }

            frag_colour = vec4(ambient + diffuseSum + specularSum, 1.0);
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

    // Associa o sampler (uTexture) a texture unit 0 uma unica vez.
    // Como o shader so tem 1 sampler, esta linha e equivalente a
    //   glUniform1i(glGetUniformLocation(Shader_programm, "uTexture"), 0);
    // feita uma vez so na inicializacao. GlActiveTexture(GL_TEXTURE0) no
    // loop garante que o textureID certo fica na unit 0.
    glUseProgram(Shader_programm);
    glUniform1i(glGetUniformLocation(Shader_programm, "uTexture"), 0);
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

    // Atenuacao padrao OpenGL: fraca → luzes visiveis ao longe
    // (Compact defaults Kc=1.0/0.7/1.8 resultam em death-black cena)
    glUniform1f(glGetUniformLocation(Shader_programm, "Kc"), 1.0f);
    glUniform1f(glGetUniformLocation(Shader_programm, "Kl"), 0.01f);
    glUniform1f(glGetUniformLocation(Shader_programm, "Kq"), 0.001f);
}

// -----------------------------
// Trajectory update
// -----------------------------
// Novo: usa a classe Trajectory completa.
// A classe ja sabe:
//   - avancar progresso (linear OU bezier cubico)
//   - calcular posicao via computeCurrentPosition()
//   - ciclicamente voltar ao comeco em cada modo
void atualizaTrajetorias(float deltaTime) {
    for (size_t i = 0; i < trajectories.size() && i < renderObjects.size(); i++) {
        Traj& traj = trajectories[i];

        // Se inativa -> nao altera a posicao do objeto
        if (!traj.isActive()) {
            continue;
        }

        // Atualiza a trajetoria (progresso + segmento, ja cuida do ciclo)
        traj.update(deltaTime);

        // Posicao interpolada (linear OU bezier, baseado no interpolation flag)
        renderObjects[i].position = traj.computeCurrentPosition();
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
    obj2.texture = "assets/tex/pixelWall.png";

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
    obj5.texture = "assets/Modelos3D/Suzanne.obj";

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

        Traj traj;
        traj.setActive(false);
        traj.setInterpolation(Trajectory::Interpolation::LINEAR);
        traj.setSpeed(1.5f);
        traj.reset();
        trajectories.push_back(traj);
    }

    // Setup lights
    numLights = 3;
    lights[0] = {"luz_principal", "Key Light", glm::vec3(5.0f, 5.0f, 5.0f), glm::vec3(1.0f), 1.0f, true};
    lights[1] = {"luz_secundaria", "Fill Light", glm::vec3(-3.0f, 3.0f, 2.0f), glm::vec3(0.8f, 0.8f, 1.0f), 0.6f, true};
    lights[2] = {"luz_traseira", "Back Light", glm::vec3(0.0f, 3.0f, -5.0f), glm::vec3(1.0f, 0.9f, 0.8f), 0.4f, true};

    // Add initial trajectory points for cube
    trajectories[0].addPoint(glm::vec3(-3.0f, 0.0f, 0.0f));
    trajectories[0].addPoint(glm::vec3(-3.0f, 2.0f, 0.0f));
    trajectories[0].addPoint(glm::vec3(-1.0f, 2.0f, 0.0f));
    trajectories[0].addPoint(glm::vec3(-1.0f, 0.0f, 0.0f));

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
        const Traj& traj = trajectories[i];

        // Pontos de controle lineares (verde)
        const auto& linearPts = traj.getPoints();
        for (size_t j = 0; j < linearPts.size(); j++) {
            Material mat;
            mat.color = glm::vec3(0.0f, 1.0f, 0.0f);
            mat.Ka = 0.2f;
            mat.Kd = 0.8f;
            mat.Ks = 0.5f;
            mat.shininess = 16.0f;

            defineMaterial(mat);

            float escala = 0.1f;
            if (i == (size_t)objeto_trajetoria_selecionado
                && j == traj.getCurrentIndex()) {
                escala = 0.15f;
            }

            transformacaoGenerica(linearPts[j], glm::vec3(0.0f), glm::vec3(escala));
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // Pontos de controle Bezier (magenta)
        const auto& bezierPts = traj.getBezierPoints();
        for (size_t j = 0; j < bezierPts.size(); j++) {
            Material mat;
            mat.color = glm::vec3(1.0f, 0.0f, 1.0f);
            mat.Ka = 0.2f;
            mat.Kd = 0.8f;
            mat.Ks = 0.5f;
            mat.shininess = 16.0f;

            defineMaterial(mat);

            float escala = 0.12f;
            // Destaca o segmento atual se for Bezier
            if (i == (size_t)objeto_trajetoria_selecionado
                && j >= traj.getCurrentSegment() * 3
                && j < traj.getCurrentSegment() * 3 + 4) {
                escala = 0.18f;
            }

            transformacaoGenerica(bezierPts[j], glm::vec3(0.0f), glm::vec3(escala));
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

        // Teclas continuas (WASD/IJKL/+/-/UO): rodam ANTES do ImGui::Render
        // para que ImGui nao roube as teclas quando nao tiver widget focado.
        processaInputContinuo();

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

            // BUG #1 fix: shader agora usa sampler2D — bindar textura
            // e dizer ao shader se este objeto tem ou nao textura.
            // O sampler sempre e associado a GL_TEXTURE0 (porque so temos
            // 1 unit no shader). Objetos sem textura caem no `baseColor =
            // objectColor` (useTexture=false).
            glUniform1i(glGetUniformLocation(Shader_programm, "useTexture"), obj.textureID != 0 ? 1 : 0);
            if (obj.textureID != 0) {
                glActiveTexture(GL_TEXTURE0);
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
                        {"Pixel Wall", "assets/tex/pixelWall.png"},
                        {"Suzanne", "assets/Modelos3D/Suzanne.png"},
                        {"Suzanne UV", "assets/Modelos3D/SuzanneUV.png"}
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
                Traj& traj = trajectories[objeto_trajetoria_selecionado];
                ImGui::Text("Trajetoria:");

                bool isActive = traj.isActive();
                if (ImGui::Checkbox("Ativa", &isActive)) {
                    traj.setActive(isActive);
                }

                bool isBezier = (traj.getInterpolation() == Trajectory::Interpolation::BEZIER);
                if (ImGui::Checkbox("Modo Bezier", &isBezier)) {
                    traj.setInterpolation(isBezier ? Trajectory::Interpolation::BEZIER : Trajectory::Interpolation::LINEAR);
                }

                float spd = traj.getSpeed();
                if (ImGui::SliderFloat("Velocidade", &spd, 0.1f, 5.0f)) {
                    traj.setSpeed(spd);
                }

                ImGui::Text("Pontos: %d", (int)traj.getPointCount());
                ImGui::Text("Bezier: %d", (int)traj.getBezierPointCount());
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
