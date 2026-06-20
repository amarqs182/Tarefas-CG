// Tarefa: Definindo trajetórias para alguns objetos
//
// Baseado no CuboIluminacao.cpp (exemplo base da disciplina)
// e no Hello3DCamera.cpp (Tarefa 4 - Câmera FPS).
//
// Este código implementa:
// - Sistema de trajetórias para objetos da cena
// - Adição de pontos de controle via teclado
// - Translação cíclica por interpolação linear
// - Salvar/carregar pontos em arquivo
// - Iluminação Phong (reutilizada do CuboIluminacao)
// - Câmera FPS (reutilizada do Hello3DCamera)
//
// Controles:
//   W / S     - mover frente / trás (câmera)
//   A / D     - mover lateral (câmera)
//   Q / E     - mover baixo / cima (câmera)
//   Mouse     - olhar em volta (yaw / pitch)
//   Scroll    - zoom (altera FOV)
//
//   1         - Adicionar ponto na posição do objeto selecionado
//   2         - Remover último ponto
//   3         - Limpar todos os pontos
//   SPACE     - Ativar/desativar trajetória
//   R         - Resetar trajetória para o início
//   UP/DOWN   - Aumentar/diminuir velocidade
//   S         - Salvar pontos em arquivo
//   L         - Carregar pontos de arquivo
//   TAB       - Trocar objeto selecionado
//   ESC       - sair
//
// Autor: aluno (amarqs182) - Unisinos 2026/1

#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <fstream>
#include <sstream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Camera.h"
#include "Trajectory.h"

using namespace std;

// -----------------------------
// Variáveis globais
// -----------------------------

GLFWwindow *Window = nullptr;
GLuint Shader_programm = 0;
GLuint Vao_cubo = 0;

int WIDTH = 1000;
int HEIGHT = 1000;

float Tempo_entre_frames = 0.0f;

// Câmera (reutiliza a classe Camera da Tarefa 4)
Camera camera(glm::vec3(0.0f, 0.0f, 5.0f));

// Trajetórias para os objetos
vector<Trajectory> trajectories(3);  // 3 objetos: cubo, esfera, pirâmide
int objeto_selecionado = 0;          // Índice do objeto selecionado
string nomes_objetos[] = {"Cubo", "Esfera", "Pirâmide"};

// Posições dos objetos
glm::vec3 posicoes_objetos[3] = {
    glm::vec3(-2.0f, 0.0f, 0.0f),  // Cubo
    glm::vec3(0.0f, 0.0f, 0.0f),   // Esfera
    glm::vec3(2.0f, 0.0f, 0.0f)    // Pirâmide
};

// Número de vértices da esfera procedural
int Num_vertices_esfera = 0;

// -----------------------------
// Callbacks de janela e entrada
// -----------------------------

void redimensionaCallback(GLFWwindow *window, int w, int h)
{
    WIDTH = w;
    HEIGHT = h;
    glViewport(0, 0, w, h);
    camera.setProjectionParams(45.0f, (float)WIDTH / (float)HEIGHT, 0.1f, 100.0f);
}

void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
    camera.processMouseMovement((float)xpos - 500.0f, 500.0f - (float)ypos);
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    camera.processMouseScroll((float)yoffset);
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Controles da câmera (movimentação contínua)
    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        if (key == GLFW_KEY_W) camera.processKeyboard(Camera::FORWARD, Tempo_entre_frames);
        if (key == GLFW_KEY_S) camera.processKeyboard(Camera::BACKWARD, Tempo_entre_frames);
        if (key == GLFW_KEY_A) camera.processKeyboard(Camera::LEFT, Tempo_entre_frames);
        if (key == GLFW_KEY_D) camera.processKeyboard(Camera::RIGHT_DIR, Tempo_entre_frames);
        if (key == GLFW_KEY_Q) camera.processKeyboard(Camera::DOWN, Tempo_entre_frames);
        if (key == GLFW_KEY_E) camera.processKeyboard(Camera::UP, Tempo_entre_frames);
    }

    // Controles da trajetória (apenas no KEY_PRESS)
    if (action == GLFW_PRESS)
    {
        Trajectory& traj = trajectories[objeto_selecionado];

        if (key == GLFW_KEY_1)
        {
            // Adiciona ponto na posição atual do objeto
            traj.addPoint(posicoes_objetos[objeto_selecionado]);
            cout << "Ponto adicionado para " << nomes_objetos[objeto_selecionado] << ": ("
                 << posicoes_objetos[objeto_selecionado].x << ", "
                 << posicoes_objetos[objeto_selecionado].y << ", "
                 << posicoes_objetos[objeto_selecionado].z << ")" << endl;
            cout << "Total de pontos: " << traj.getPointCount() << endl;
        }

        if (key == GLFW_KEY_2)
        {
            if (traj.getPointCount() > 0)
            {
                traj.removePoint(traj.getPointCount() - 1);
                cout << "Último ponto removido. Total: " << traj.getPointCount() << endl;
            }
        }

        if (key == GLFW_KEY_3)
        {
            traj.clearPoints();
            cout << "Todos os pontos foram removidos." << endl;
        }

        if (key == GLFW_KEY_SPACE)
        {
            traj.setActive(!traj.isActive());
            cout << "Trajetória " << (traj.isActive() ? "ATIVADA" : "DESATIVADA")
                 << " para " << nomes_objetos[objeto_selecionado] << endl;
        }

        if (key == GLFW_KEY_R)
        {
            traj.reset();
            cout << "Trajetória resetada." << endl;
        }

        if (key == GLFW_KEY_UP)
        {
            traj.setSpeed(traj.getSpeed() + 0.5f);
            cout << "Velocidade: " << traj.getSpeed() << endl;
        }

        if (key == GLFW_KEY_DOWN)
        {
            float newSpeed = traj.getSpeed() - 0.5f;
            if (newSpeed < 0.1f) newSpeed = 0.1f;
            traj.setSpeed(newSpeed);
            cout << "Velocidade: " << traj.getSpeed() << endl;
        }

        if (key == GLFW_KEY_S)
        {
            string filename = "trajectory_" + nomes_objetos[objeto_selecionado] + ".txt";
            if (traj.saveToFile(filename))
                cout << "Pontos salvos em '" << filename << "'" << endl;
            else
                cout << "Erro ao salvar pontos!" << endl;
        }

        if (key == GLFW_KEY_L)
        {
            string filename = "trajectory_" + nomes_objetos[objeto_selecionado] + ".txt";
            if (traj.loadFromFile(filename))
                cout << "Pontos carregados de '" << filename << "'. Total: " << traj.getPointCount() << endl;
            else
                cout << "Erro ao carregar pontos!" << endl;
        }

        if (key == GLFW_KEY_TAB)
        {
            objeto_selecionado = (objeto_selecionado + 1) % 3;
            cout << "Objeto selecionado: " << nomes_objetos[objeto_selecionado] << endl;
        }
    }
}

// -----------------------------
// Inicialização do OpenGL
// -----------------------------

void inicializaOpenGL()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    Window = glfwCreateWindow(WIDTH, HEIGHT,
        "CG 2026/1 - Tarefa: Trajetória de Objetos", nullptr, nullptr);
    glfwMakeContextCurrent(Window);

    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    glfwSetFramebufferSizeCallback(Window, redimensionaCallback);
    glfwSetCursorPosCallback(Window, mouse_callback);
    glfwSetScrollCallback(Window, scroll_callback);
    glfwSetKeyCallback(Window, key_callback);
    glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    camera.setProjectionParams(45.0f, (float)WIDTH / (float)HEIGHT, 0.1f, 100.0f);

    cout << "Placa de vídeo: " << glGetString(GL_RENDERER) << endl;
    cout << "Versão do OpenGL: " << glGetString(GL_VERSION) << endl;
}

// -----------------------------
// Inicialização da geometria
// -----------------------------

void inicializaCubo()
{
    // Cubo com normais para iluminação Phong
    // Formato: posição(3) + normal(3) = 6 floats por vértice
    float points[] = {
        // posição              // normal

        // Frente (0,0,1)
        0.5f, 0.5f, 0.5f,      0.0f, 0.0f, 1.0f,
        0.5f, -0.5f, 0.5f,     0.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, 0.5f,    0.0f, 0.0f, 1.0f,
        0.5f, 0.5f, 0.5f,      0.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, 0.5f,    0.0f, 0.0f, 1.0f,
        -0.5f, 0.5f, 0.5f,     0.0f, 0.0f, 1.0f,

        // Trás (0,0,-1)
        0.5f, 0.5f, -0.5f,     0.0f, 0.0f, -1.0f,
        0.5f, -0.5f, -0.5f,    0.0f, 0.0f, -1.0f,
        -0.5f, -0.5f, -0.5f,   0.0f, 0.0f, -1.0f,
        0.5f, 0.5f, -0.5f,     0.0f, 0.0f, -1.0f,
        -0.5f, -0.5f, -0.5f,   0.0f, 0.0f, -1.0f,
        -0.5f, 0.5f, -0.5f,    0.0f, 0.0f, -1.0f,

        // Esquerda (-1,0,0)
        -0.5f, -0.5f, 0.5f,    -1.0f, 0.0f, 0.0f,
        -0.5f, 0.5f, 0.5f,     -1.0f, 0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,   -1.0f, 0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,   -1.0f, 0.0f, 0.0f,
        -0.5f, 0.5f, -0.5f,    -1.0f, 0.0f, 0.0f,
        -0.5f, 0.5f, 0.5f,     -1.0f, 0.0f, 0.0f,

        // Direita (1,0,0)
        0.5f, -0.5f, 0.5f,     1.0f, 0.0f, 0.0f,
        0.5f, 0.5f, 0.5f,      1.0f, 0.0f, 0.0f,
        0.5f, -0.5f, -0.5f,    1.0f, 0.0f, 0.0f,
        0.5f, -0.5f, -0.5f,    1.0f, 0.0f, 0.0f,
        0.5f, 0.5f, -0.5f,     1.0f, 0.0f, 0.0f,
        0.5f, 0.5f, 0.5f,      1.0f, 0.0f, 0.0f,

        // Baixo (0,-1,0)
        -0.5f, -0.5f, 0.5f,    0.0f, -1.0f, 0.0f,
        0.5f, -0.5f, 0.5f,     0.0f, -1.0f, 0.0f,
        0.5f, -0.5f, -0.5f,    0.0f, -1.0f, 0.0f,
        0.5f, -0.5f, -0.5f,    0.0f, -1.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,   0.0f, -1.0f, 0.0f,
        -0.5f, -0.5f, 0.5f,    0.0f, -1.0f, 0.0f,

        // Cima (0,1,0)
        -0.5f, 0.5f, 0.5f,     0.0f, 1.0f, 0.0f,
        0.5f, 0.5f, 0.5f,      0.0f, 1.0f, 0.0f,
        0.5f, 0.5f, -0.5f,     0.0f, 1.0f, 0.0f,
        0.5f, 0.5f, -0.5f,     0.0f, 1.0f, 0.0f,
        -0.5f, 0.5f, -0.5f,    0.0f, 1.0f, 0.0f,
        -0.5f, 0.5f, 0.5f,     0.0f, 1.0f, 0.0f
    };

    GLuint VBO;
    glGenVertexArrays(1, &Vao_cubo);
    glGenBuffers(1, &VBO);

    glBindVertexArray(Vao_cubo);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);

    // Atributo 0: Posição
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    // Atributo 1: Normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
}

// -----------------------------
// Shaders (Phong com atenuação)
// -----------------------------

GLuint compilaShader(const char *source, GLenum type)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    return shader;
}

void inicializaShaders()
{
    const char *vertex_shader = R"(
        #version 450

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

            // Normal matrix para transformação correta de normais
            normal = mat3(transpose(inverse(model))) * vertex_normal;

            gl_Position = proj * view * worldPos;
        }
    )";

    const char *fragment_shader = R"(
        #version 450

        in vec3 fragPos;
        in vec3 normal;

        out vec4 frag_colour;

        uniform vec3 lightPos;
        uniform vec3 viewPos;

        uniform vec3 lightColor;
        uniform vec3 objectColor;

        uniform float Ka;       // Coeficiente de reflexão ambiente
        uniform float Kd;       // Coeficiente de reflexão difusa
        uniform float Ks;       // Coeficiente de reflexão especular
        uniform float shininess; // Expoente especular

        // Constantes da equação de atenuação
        uniform float Kc;       // Constante
        uniform float Kl;       // Linear
        uniform float Kq;       // Quadrática

        void main()
        {
            vec3 N = normalize(normal);
            vec3 L = normalize(lightPos - fragPos);
            vec3 V = normalize(viewPos - fragPos);
            vec3 R = normalize(reflect(-L, N)); // Phong tradicional

            // Calcula a distância entre a luz e o fragmento
            float d = length(lightPos - fragPos);

            // Calcula o fator de atenuação
            float attenuation = 1.0 / (Kc + Kl * d + Kq * (d * d));

            // Ambiente
            vec3 ambient = Ka * lightColor;

            // Difusa
            float diff = max(dot(N, L), 0.0);
            vec3 diffuse = Kd * diff * lightColor;

            // Especular (Phong tradicional)
            float spec = pow(max(dot(V, R), 0.0), shininess);
            vec3 specular = Ks * spec * lightColor;

            // Aplica atenuação
            diffuse *= attenuation;
            specular *= attenuation;

            vec3 result = (ambient + diffuse) * objectColor + specular;

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
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
}

// -----------------------------
// Transformação de modelo
// -----------------------------

void transformacaoGenerica(float Tx, float Ty, float Tz,
                           float Sx, float Sy, float Sz)
{
    glm::mat4 transform(1.0f);

    transform = glm::translate(transform, glm::vec3(Tx, Ty, Tz));
    transform = glm::scale(transform, glm::vec3(Sx, Sy, Sz));

    GLuint loc = glGetUniformLocation(Shader_programm, "model");
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(transform));
}

// -----------------------------
// Câmera (matriz de visualização e projeção)
// -----------------------------

void inicializaCamera()
{
    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 proj = camera.getProjectionMatrix();

    glUniformMatrix4fv(glGetUniformLocation(Shader_programm, "view"),
                       1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(Shader_programm, "proj"),
                       1, GL_FALSE, glm::value_ptr(proj));
}

// -----------------------------
// Material e iluminação
// -----------------------------

void defineMaterial(float r, float g, float b,
                    float ka, float kd, float ks,
                    float shininess)
{
    glUniform3f(glGetUniformLocation(Shader_programm, "objectColor"),
                r, g, b);

    glUniform1f(glGetUniformLocation(Shader_programm, "Ka"), ka);
    glUniform1f(glGetUniformLocation(Shader_programm, "Kd"), kd);
    glUniform1f(glGetUniformLocation(Shader_programm, "Ks"), ks);
    glUniform1f(glGetUniformLocation(Shader_programm, "shininess"), shininess);
}

void defineIluminacao()
{
    glm::vec3 lightPos(2.0f, 2.0f, 2.0f);

    glUniform3fv(glGetUniformLocation(Shader_programm, "lightPos"),
                1, glm::value_ptr(lightPos));

    glUniform3fv(glGetUniformLocation(Shader_programm, "viewPos"),
                1, glm::value_ptr(camera.getPosition()));

    glUniform3f(glGetUniformLocation(Shader_programm, "lightColor"),
                1.0f, 1.0f, 1.0f);

    // Atenuação (cobertura de ~50 unidades)
    glUniform1f(glGetUniformLocation(Shader_programm, "Kc"), 1.0f);
    glUniform1f(glGetUniformLocation(Shader_programm, "Kl"), 0.09f);
    glUniform1f(glGetUniformLocation(Shader_programm, "Kq"), 0.032f);
}

// -----------------------------
// Renderização
// -----------------------------

void desenhaPontosControle()
{
    // Desenha os pontos de controle como cubos pequenos
    for (int i = 0; i < 3; i++)
    {
        const Trajectory& traj = trajectories[i];
        if (traj.getPointCount() == 0)
            continue;

        for (size_t j = 0; j < traj.getPointCount(); j++)
        {
            glm::vec3 point = traj.getPoint(j);

            // Material verde para pontos de controle
            defineMaterial(0.0f, 1.0f, 0.0f, 0.2f, 0.8f, 0.5f, 16.0f);

            // Cubo menor (0.1) para representar o ponto
            float escala = 0.1f;
            if (j == traj.getCurrentIndex() && i == objeto_selecionado)
                escala = 0.15f;  // Destaca o ponto atual do objeto selecionado

            transformacaoGenerica(point.x, point.y, point.z, escala, escala, escala);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
    }
}

void inicializaRenderizacao()
{
    float tempo_anterior = (float)glfwGetTime();

    glEnable(GL_DEPTH_TEST);

    // Adiciona pontos iniciais de exemplo para o cubo
    trajectories[0].addPoint(glm::vec3(-2.0f, 0.0f, 0.0f));
    trajectories[0].addPoint(glm::vec3(-2.0f, 2.0f, 0.0f));
    trajectories[0].addPoint(glm::vec3(0.0f, 2.0f, 0.0f));
    trajectories[0].addPoint(glm::vec3(0.0f, 0.0f, 0.0f));
    trajectories[0].setSpeed(1.5f);

    cout << "\n=== Controles ===" << endl;
    cout << "WASD/QE - Mover câmera" << endl;
    cout << "Mouse - Olhar em volta" << endl;
    cout << "TAB - Trocar objeto selecionado" << endl;
    cout << "1 - Adicionar ponto na posição do objeto" << endl;
    cout << "2 - Remover último ponto" << endl;
    cout << "3 - Limpar todos os pontos" << endl;
    cout << "SPACE - Ativar/desativar trajetória" << endl;
    cout << "R - Resetar trajetória" << endl;
    cout << "UP/DOWN - Ajustar velocidade" << endl;
    cout << "S - Salvar pontos em arquivo" << endl;
    cout << "L - Carregar pontos de arquivo" << endl;
    cout << "ESC - Sair" << endl;
    cout << "==================\n" << endl;

    while (!glfwWindowShouldClose(Window))
    {
        float tempo_atual = (float)glfwGetTime();
        Tempo_entre_frames = tempo_atual - tempo_anterior;
        tempo_anterior = tempo_atual;

        // Atualiza trajetórias
        for (int i = 0; i < 3; i++)
        {
            trajectories[i].update(Tempo_entre_frames);
            if (trajectories[i].isActive())
                posicoes_objetos[i] = trajectories[i].getCurrentPosition();
        }

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(Shader_programm);
        inicializaCamera();
        defineIluminacao();

        glBindVertexArray(Vao_cubo);

        // 1. Desenha o Cubo (objeto 0)
        defineMaterial(1.0f, 0.6f, 0.2f, 0.1f, 0.7f, 1.0f, 32.0f);
        transformacaoGenerica(posicoes_objetos[0].x, posicoes_objetos[0].y, posicoes_objetos[0].z,
                             1.0f, 1.0f, 1.0f);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // 2. Desenha a Esfera (objeto 1) - representada como cubo menor
        defineMaterial(0.2f, 0.8f, 0.2f, 0.1f, 0.7f, 1.0f, 32.0f);
        transformacaoGenerica(posicoes_objetos[1].x, posicoes_objetos[1].y, posicoes_objetos[1].z,
                             0.7f, 0.7f, 0.7f);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // 3. Desenha a Pirâmide (objeto 2) - representada como cubo menor
        defineMaterial(0.2f, 0.2f, 1.0f, 0.1f, 0.7f, 1.0f, 32.0f);
        transformacaoGenerica(posicoes_objetos[2].x, posicoes_objetos[2].y, posicoes_objetos[2].z,
                             0.5f, 0.5f, 0.5f);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // 4. Desenha pontos de controle
        desenhaPontosControle();

        glfwSwapBuffers(Window);
        glfwPollEvents();
    }

    glfwTerminate();
}

// -----------------------------
// Função principal
// -----------------------------

int main()
{
    inicializaOpenGL();
    inicializaCubo();
    inicializaShaders();
    inicializaRenderizacao();
    return 0;
}