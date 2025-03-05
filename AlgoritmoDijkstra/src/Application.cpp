#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <queue>
#include <map>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"  // Para cargar imágenes
#include <sstream>
struct Nodo {
    float x, y;
    char etiqueta;
    std::vector<std::pair<int, float>> vecinos; // (Índice del nodo vecino, peso)
};

std::vector<Nodo> nodos;
std::vector<int> caminoMasCorto;
std::map<char, int> nodoIndices;
std::vector<std::tuple<int, int, float>> aristas;
GLuint texturaFuente;
int nodoSeleccionado = -1;
int nodoInicio = -1;  // Nodo de inicio seleccionado
int nodoDestino = -1; // Nodo de destino seleccionado
// Función para cargar la fuente
void cargarFuente(const char* ruta) {
    int ancho, alto, canales;
    unsigned char* data = stbi_load(ruta, &ancho, &alto, &canales, 4);
    glGenTextures(1, &texturaFuente);
    glBindTexture(GL_TEXTURE_2D, texturaFuente);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ancho, alto, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(data);
}

// Dibujar texto con la fuente
void dibujarTexto(float x, float y, char letra) {
    float u = (letra % 16) / 16.0f;
    float v = (letra / 6) / 6.0f;
    float sizeX = 1.0f / 16.0f;
    float sizeY = 1.0f / 6.0f;

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texturaFuente);
    glBegin(GL_QUADS);
    glTexCoord2f(u, v); glVertex2f(x, y);
    glTexCoord2f(u + sizeX, v); glVertex2f(x + 0.05f, y);
    glTexCoord2f(u + sizeX, v + sizeY); glVertex2f(x + 0.05f, y + 0.05f);
    glTexCoord2f(u, v + sizeY); glVertex2f(x, y + 0.05f);
    glEnd();
    glDisable(GL_TEXTURE_2D);
}
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
// Dibujar nodos y etiquetas
void dibujarNodos() {
    for (const auto& nodo : nodos) {
        glColor3f(0.0f, 1.0f, 0.0f);
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(nodo.x, nodo.y);
        for (int i = 0; i <= 20; i++) {
            float angle = i * 2.0f * M_PI / 20;
            glVertex2f(nodo.x + cos(angle) * 0.05f, nodo.y + sin(angle) * 0.05f);
        }
        glEnd();
        dibujarTexto(nodo.x, nodo.y, nodo.etiqueta);
    }
}

// Dibujar conexiones con distancias
void dibujarAristas() {
    glLineWidth(2.0f);

    for (auto& arista : aristas) {
        int nodoA = std::get<0>(arista);
        int nodoB = std::get<1>(arista);
        float distancia = std::get<2>(arista);

        // Verificar si el vector tiene suficientes elementos para recorrerlo
        if (caminoMasCorto.size() < 2) {
            glColor3f(1.0f, 1.0f, 1.0f); // Dibuja en blanco si no hay camino
            glLineWidth(2.0f);
        }
        else {
            // Verificar si la arista está en el camino más corto
            bool esCaminoCorto = false;
            for (size_t i = 0; i < caminoMasCorto.size() - 1; i++) {
                if ((nodoA == caminoMasCorto[i] && nodoB == caminoMasCorto[i + 1]) ||
                    (nodoB == caminoMasCorto[i] && nodoA == caminoMasCorto[i + 1])) {
                    esCaminoCorto = true;
                    break;
                }
            }

            if (esCaminoCorto) {
                glColor3f(1.0f, 0.0f, 0.0f); // Rojo para el camino más corto
                glLineWidth(4.0f);
            }
            else {
                glColor3f(1.0f, 1.0f, 1.0f); // Blanco para caminos normales
                glLineWidth(2.0f);
            }
        }

        glBegin(GL_LINES);
        glVertex2f(nodos[nodoA].x, nodos[nodoA].y);
        glVertex2f(nodos[nodoB].x, nodos[nodoB].y);
        glEnd();
    }
}

// Algoritmo de Dijkstra
std::vector<int> dijkstra(int inicio, int destino) {
    std::vector<float> dist(nodos.size(), FLT_MAX);
    std::vector<int> prev(nodos.size(), -1);
    std::priority_queue<std::pair<float, int>, std::vector<std::pair<float, int>>, std::greater<>> pq;

    dist[inicio] = 0;
    pq.push({ 0, inicio });

    while (!pq.empty()) {
        int u = pq.top().second;
        pq.pop();

        if (u == destino) break;

        for (auto& vecino : nodos[u].vecinos) {
            int v = vecino.first;
            float peso = vecino.second;

            if (dist[u] + peso < dist[v]) {
                dist[v] = dist[u] + peso;
                prev[v] = u;
                pq.push({ dist[v], v });
            }
        }
    }

    // Reconstrucción del camino
    std::vector<int> camino;
    for (int at = destino; at != -1; at = prev[at]) {
        camino.push_back(at);
    }
    std::reverse(camino.begin(), camino.end());

    return camino;
}
const int screenWidth = 1280;  // Ajusta según la resolución de tu ventana
const int screenHeight = 720;
// Manejo de clics del mouse
void mouse_callback(GLFWwindow* window, int button, int action, int mods) {
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    float x = (xpos / width) * 2.0f - 1.0f;
    float y = 1.0f - (ypos / height) * 2.0f;
    if (action == GLFW_PRESS) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        // Convertir coordenadas de pantalla a coordenadas normalizadas OpenGL
        float x = (xpos / screenWidth) * 2.0f - 1.0f;
        float y = 1.0f - (ypos / screenHeight) * 2.0f;

        int nodoCercano = -1;
        float minDist = 0.05f; // Radio de selección

        // Buscar el nodo más cercano
        for (int i = 0; i < nodos.size(); i++) {
            float dist = sqrt(pow(nodos[i].x - x, 2) + pow(nodos[i].y - y, 2));
            if (dist < minDist) {
                nodoCercano = i;
                minDist = dist;
            }
        }

        if (nodoCercano != -1) {
            if (button == GLFW_MOUSE_BUTTON_LEFT) {
                // Selección de nodos para aristas
                if (nodoSeleccionado == -1) {
                    nodoSeleccionado = nodoCercano;
                }
                else if (nodoSeleccionado != nodoCercano) {
                    float distancia = sqrt(pow(nodos[nodoSeleccionado].x - nodos[nodoCercano].x, 2) +
                        pow(nodos[nodoSeleccionado].y - nodos[nodoCercano].y, 2));

                    nodos[nodoSeleccionado].vecinos.push_back({ nodoCercano, distancia });
                    nodos[nodoCercano].vecinos.push_back({ nodoSeleccionado, distancia });
                    aristas.push_back(std::make_tuple(nodoSeleccionado, nodoCercano, distancia));

                    nodoSeleccionado = nodoCercano; // Para permitir seguir conectando
                }
            }
            else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
                // Seleccionar nodo de inicio y destino
                if (nodoInicio == -1) {
                    nodoInicio = nodoCercano;
                    std::cout << "Nodo de inicio seleccionado: " << char('A' + nodoInicio) << std::endl;
                }
                else {
                    nodoDestino = nodoCercano;
                    std::cout << "Nodo de destino seleccionado: " << char('A' + nodoDestino) << std::endl;
                }
            }
        }
    }

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        int nodoCercano = -1;
        for (size_t i = 0; i < nodos.size(); i++) {
            float dx = nodos[i].x - x;
            float dy = nodos[i].y - y;
            if (sqrt(dx * dx + dy * dy) < 0.05f) {
                nodoCercano = i;
                break;
            }
        }

        if (nodoCercano == -1) {
            char etiqueta = 'A' + nodos.size();
            nodos.push_back({ x, y, etiqueta });
            nodoIndices[etiqueta] = nodos.size() - 1;
            std::cout << "Nodo " << etiqueta << " agregado en (" << x << ", " << y << ")\n";
        }
        else {
            if (nodoSeleccionado == -1) {
                nodoSeleccionado = nodoCercano;
            }
            else {
                if (nodoSeleccionado != nodoCercano) {
                    float distancia = sqrt(pow(nodos[nodoSeleccionado].x - nodos[nodoCercano].x, 2) +
                        pow(nodos[nodoSeleccionado].y - nodos[nodoCercano].y, 2)) * 800;
                    nodos[nodoSeleccionado].vecinos.push_back({ nodoCercano, distancia });
                    nodos[nodoCercano].vecinos.push_back({ nodoSeleccionado, distancia });
                    aristas.push_back(std::make_tuple(nodoSeleccionado, nodoCercano, distancia));
                    std::cout << "Arista creada: " << nodos[nodoSeleccionado].etiqueta << " - "
                        << nodos[nodoCercano].etiqueta << " Distancia: " << distancia << "\n";
                }
                nodoSeleccionado = nodoCercano;
            }
        }
    }
}
void resaltarCamino(const std::vector<int>& camino);
//Manejo teclas 
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_ENTER) {
            if (nodoInicio != -1 && nodoDestino != -1) {
                std::cout << "Ejecutando Dijkstra desde " << char('A' + nodoInicio)
                    << " hasta " << char('A' + nodoDestino) << std::endl;

                std::vector<int> camino = dijkstra(nodoInicio, nodoDestino);

                if (camino.empty()) {
                    std::cout << " No se encontró camino entre los nodos seleccionados.\n";
                }
                else {
                    std::cout << "Camino más corto: ";
                    for (int nodo : camino) {
                        std::cout << char('A' + nodo) << " ";
                    }
                    std::cout << std::endl;

                    caminoMasCorto = camino;
                }
            }
            else {
                std::cout << "Debes seleccionar un nodo de inicio y un nodo de destino.\n";
            }
        }
    }
}
//resaltar camino
void resaltarCamino(const std::vector<int>& camino) {
    if (camino.size() < 2) return; // No hay camino

    glLineWidth(3.0f);
    glColor3f(1.0f, 0.0f, 0.0f); // Rojo para el camino más corto

    for (size_t i = 0; i < camino.size() - 1; i++) {
        int a = camino[i];
        int b = camino[i + 1];

        glBegin(GL_LINES);
        glVertex2f(nodos[a].x, nodos[a].y);
        glVertex2f(nodos[b].x, nodos[b].y);
        glEnd();
    }

    glLineWidth(1.0f); // Restablecer grosor de línea
}

// Main
int main() {
    if (!glfwInit()) return -1;

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Simulador Dijkstra", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
    glewInit();
    glfwSetMouseButtonCallback(window, mouse_callback);
    glfwSetKeyCallback(window, key_callback);
    cargarFuente("fuente.png");

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);
        dibujarAristas();
        dibujarNodos();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}