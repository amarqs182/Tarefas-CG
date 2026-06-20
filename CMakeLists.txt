cmake_minimum_required(VERSION 3.10)
project(CGCCHIB)

# Define o padrão do C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Ativa o FetchContent
include(FetchContent)

# Configura o FetchContent para baixar e compilar GLFW automaticamente
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.4  # Define a versão desejada da GLFW
)

# Configura o FetchContent para baixar a GLM automaticamente
FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG master  # Define a versão desejada da GLM
)

# Faz o download e compila as bibliotecas
FetchContent_MakeAvailable(glfw glm)

# Configura o FetchContent para baixar a stb_image automaticamente
FetchContent_Declare(
  stb_image
  GIT_REPOSITORY https://github.com/nothings/stb.git
  GIT_TAG master
)

# Configura o FetchContent para baixar o ImGui automaticamente
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.91.8
)

FetchContent_MakeAvailable(stb_image imgui)

# Adiciona as pastas de cabeçalhos
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/common)
include_directories(${CMAKE_SOURCE_DIR}/include/glad)
include_directories(${glm_SOURCE_DIR})
include_directories(${stb_image_SOURCE_DIR})

# Adiciona diretórios do ImGui
include_directories(${imgui_SOURCE_DIR})
include_directories(${imgui_SOURCE_DIR}/backends)


# Lista de exemplos/exercícios podem ser colocados aqui também
set(EXERCISES
    Hello3D
    Hello3DCamera
    TriangleTex
    SpherePhong
    CuboIluminacao
    TrajectoryDemo
    MainDiorama
)

# Arquivos adicionais necessários para a TrajectoryDemo
set(TRAJECTORY_SRC
    src/Trajectory.cpp
)

# Arquivos adicionais necessários para o MainDiorama
set(MAINDIORAMA_SRC
    src/SceneParser.cpp
)

add_compile_options(-Wno-pragmas)

# Define as bibliotecas para cada sistema operacional
if(WIN32)
    set(OPENGL_LIBS opengl32)
elseif(APPLE)
    find_library(OpenGL_LIBRARY OpenGL)
    set(OPENGL_LIBS ${OpenGL_LIBRARY})
else()
    find_package(OpenGL REQUIRED)
    set(OPENGL_LIBS ${OPENGL_gl_LIBRARY})
endif()

# Caminho esperado para a GLAD
set(GLAD_C_FILE "${CMAKE_SOURCE_DIR}/Common/glad.c")

# Verifica se os arquivos da GLAD estão no lugar
if (NOT EXISTS ${GLAD_C_FILE})
    message(FATAL_ERROR "Arquivo glad.c não encontrado! Baixe a GLAD manualmente em https://glad.dav1d.de/ e coloque glad.h em include/glad/ e glad.c em common/")
endif()

# Cria os executáveis
foreach(EXERCISE ${EXERCISES})
    if(${EXERCISE} STREQUAL "TrajectoryDemo")
        # TrajectoryDemo precisa do arquivo Trajectory.cpp e Camera.cpp
        add_executable(${EXERCISE} src/${EXERCISE}.cpp src/Trajectory.cpp src/Camera.cpp ${GLAD_C_FILE})
    elseif(${EXERCISE} STREQUAL "Hello3DCamera")
        # Hello3DCamera precisa do arquivo Camera.cpp
        add_executable(${EXERCISE} src/${EXERCISE}.cpp src/Camera.cpp ${GLAD_C_FILE})
    elseif(${EXERCISE} STREQUAL "MainDiorama")
        # MainDiorama usa Camera.cpp, ObjParser.cpp e ImGui
        set(IMGUI_SOURCES
            ${imgui_SOURCE_DIR}/imgui.cpp
            ${imgui_SOURCE_DIR}/imgui_draw.cpp
            ${imgui_SOURCE_DIR}/imgui_tables.cpp
            ${imgui_SOURCE_DIR}/imgui_widgets.cpp
            ${imgui_SOURCE_DIR}/imgui_demo.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
        )
        add_executable(${EXERCISE} src/${EXERCISE}.cpp src/Camera.cpp src/ObjParser.cpp ${IMGUI_SOURCES} ${GLAD_C_FILE})
    else()
        add_executable(${EXERCISE} src/${EXERCISE}.cpp ${GLAD_C_FILE})
    endif()
    target_include_directories(${EXERCISE} PRIVATE ${CMAKE_SOURCE_DIR}/include/glad ${glm_SOURCE_DIR} ${stb_image_SOURCE_DIR})
    target_link_libraries(${EXERCISE} glfw ${OPENGL_LIBS})
endforeach()
