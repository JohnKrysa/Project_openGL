#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <algorithm>


enum GameState { MENU, SETTINGS, GAME, GAME_OVER };
GameState currentState = MENU;

enum AttackType { NORMAL, LASER, BOOMERANG, LONG_LASER, TILE_DMG };

float cubeOffsetX = 0.0f;
float cubeOffsetY = 0.0f;

const float GRID_STEP  = 0.4f;
const float GRID_LIMIT = 0.8f;
const float gridCoords[5] = {-0.8f, -0.4f, 0.0f, 0.4f, 0.8f};

struct Attack {
    AttackType type;
    float x, y;
    float dx, dy;
    float r, g, b;
    float timer = 0.0f;
    bool returning = false;
    bool active = true;
    float width = 0.3f;
    float height = 0.3f;
};
std::vector<Attack> attacks;

float gameTime = 0.0f;
float lastAttackSpawn = 0.0f;
float currentSpawnDelay = 1.2f;
float globalSpeedMultiplier = 1.0f;

void ResetGame() {
    cubeOffsetX = 0.0f;
    cubeOffsetY = 0.0f;
    attacks.clear();
    gameTime = 0.0f;
    lastAttackSpawn = 0.0f;
    currentSpawnDelay = 1.2f;
    globalSpeedMultiplier = 1.0f;
}

void SpawnAttack(float playerX, float playerY) {
    int attackType = rand() % 14;
    float speed = 1.2f * globalSpeedMultiplier;
    
    float r = 0.9f, g = 0.1f, b = 0.1f; 

    if (attackType < 4) { 
        attacks.push_back({NORMAL, gridCoords[rand() % 5], 1.4f, 0.0f, -speed, r, g, b, 0.0f, false, true, 0.3f, 0.3f});
    } 
    else if (attackType < 6) { 
        float ry = gridCoords[rand() % 5];
        attacks.push_back({LASER, 0.0f, ry, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, false, false, 2.8f, 0.15f});
    } 
    else if (attackType < 8) { 
        float rx = gridCoords[rand() % 5];
        attacks.push_back({BOOMERANG, rx, 1.4f, 0.0f, -speed * 1.2f, 0.2f, 0.8f, 0.9f, 0.0f, false, true, 0.3f, 0.3f});
    }
    else if (attackType < 10) { 
        float ry = gridCoords[rand() % 5];
        attacks.push_back({LONG_LASER, 0.0f, ry, 0.0f, 0.0f, 0.5f, 0.4f, 0.0f, 0.0f, false, false, 2.8f, 0.15f});
    }
    else if (attackType < 12) {
        float rx = gridCoords[rand() % 5];
        float ry = gridCoords[rand() % 5];
        bool big = (rand() % 2 == 0);
        float size = big ? 0.8f : 0.4f;
        attacks.push_back({TILE_DMG, rx, ry, 0.0f, 0.0f, 0.4f, 0.1f, 0.1f, 0.0f, false, false, size, size});
    }
    else {
        attacks.push_back({NORMAL, playerX, 1.4f, 0.0f, -speed * 1.5f, 0.8f, 0.1f, 0.8f, 0.0f, false, true, 0.3f, 0.3f});
    }
}

bool checkCollision(float ax, float ay, float aw, float ah) {
    return std::abs(cubeOffsetX - ax) < (0.15f + aw / 2.0f) && 
           std::abs(cubeOffsetY - ay) < (0.15f + ah / 2.0f);
}

static unsigned int CompileShader(unsigned int type, const std::string& source) {
    unsigned int id = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);

    int result;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE) {
        int length;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
        char* message = (char*)alloca(length * sizeof(char));
        glGetShaderInfoLog(id, length, &length, message);
        std::cout << "Failed to compile shader!\n" << message << std::endl;
        glDeleteShader(id);
        return 0;
    }
    return id;
}

static unsigned int CreateShader(const std::string& vertexShader, const std::string& fragmentShader) {
    unsigned int program = glCreateProgram();
    unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertexShader);
    unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glValidateProgram(program);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}

std::string vertexShaderSource = R"(#version 330 core 
layout (location = 0) in vec3 aPos;
uniform vec2 offset;
uniform vec2 scale;
uniform vec3 color;
out vec3 fragColor;
void main() {
    vec2 pos = (aPos.xy * scale) + offset;
    gl_Position = vec4(pos * 0.7, 0.0, 1.0); 
    fragColor = color;
})";

std::string fragmentShaderSource = R"(#version 330 core
in vec3 fragColor;
out vec4 FragColor;
void main() {
    FragColor = vec4(fragColor, 1.0);
})";

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);
        int width, height;
        glfwGetWindowSize(window, &width, &height);

        float x = (mouseX / width) * 2.0f - 1.0f;
        float y = -((mouseY / height) * 2.0f - 1.0f);

        if (currentState == MENU) {
            if (x > -0.4f && x < 0.4f && y > 0.3f && y < 0.6f) {
                ResetGame();
                currentState = GAME;
            } else if (x > -0.4f && x < 0.4f && y > -0.15f && y < 0.15f) {
                currentState = SETTINGS;
            } else if (x > -0.4f && x < 0.4f && y > -0.6f && y < -0.3f) {
                glfwSetWindowShouldClose(window, true);
            }
        } else if (currentState == SETTINGS) {
            if (x > -0.4f && x < 0.4f && y > -0.2f && y < 0.2f) currentState = MENU;
        }
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        if (currentState == GAME) {
            if (key == GLFW_KEY_RIGHT && cubeOffsetX < GRID_LIMIT - 0.01f) cubeOffsetX += GRID_STEP;
            if (key == GLFW_KEY_LEFT && cubeOffsetX > -GRID_LIMIT + 0.01f) cubeOffsetX -= GRID_STEP;
            if (key == GLFW_KEY_UP && cubeOffsetY < GRID_LIMIT - 0.01f) cubeOffsetY += GRID_STEP;
            if (key == GLFW_KEY_DOWN && cubeOffsetY > -GRID_LIMIT + 0.01f) cubeOffsetY -= GRID_STEP;
        }

        if (currentState == GAME_OVER && key == GLFW_KEY_R) {
            ResetGame();
            currentState = GAME;
        }

        if (key == GLFW_KEY_ESCAPE) currentState = MENU;
    }
}

int main() {
    srand(time(NULL)); //random seed

    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(900, 900, "Grid Dodge", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    if (glewInit() != GLEW_OK) return -1;

    unsigned int shaderProgram = CreateShader(vertexShaderSource, fragmentShaderSource);

    float quadVertices[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
        -0.5f,  0.5f, 0.0f,
         0.5f,  0.5f, 0.0f
    };
    unsigned int quadVAO, quadVBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    float gridVertices[] = {
        -0.6f, -1.0f, 0.0f,  -0.6f, 1.0f, 0.0f,
        -0.2f, -1.0f, 0.0f,  -0.2f, 1.0f, 0.0f,
         0.2f, -1.0f, 0.0f,   0.2f, 1.0f, 0.0f,
         0.6f, -1.0f, 0.0f,   0.6f, 1.0f, 0.0f,
        -1.0f, -0.6f, 0.0f,   1.0f, -0.6f, 0.0f,
        -1.0f, -0.2f, 0.0f,   1.0f, -0.2f, 0.0f,
        -1.0f,  0.2f, 0.0f,   1.0f,  0.2f, 0.0f,
        -1.0f,  0.6f, 0.0f,   1.0f,  0.6f, 0.0f
    };
    unsigned int gridVAO, gridVBO;
    glGenVertexArrays(1, &gridVAO);
    glGenBuffers(1, &gridVBO);
    glBindVertexArray(gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(gridVertices), gridVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetKeyCallback(window, key_callback);

    int offsetLoc = glGetUniformLocation(shaderProgram, "offset");
    int scaleLoc  = glGetUniformLocation(shaderProgram, "scale");
    int colorLoc  = glGetUniformLocation(shaderProgram, "color");

    float lastFrame = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        if (currentState == GAME) {
            gameTime += deltaTime;
            
            globalSpeedMultiplier = 1.0f + (gameTime * 0.05f); 
            currentSpawnDelay = std::max(0.3f, 1.2f - (gameTime * 0.02f));

            if (currentFrame - lastAttackSpawn > currentSpawnDelay) {
                SpawnAttack(cubeOffsetX, cubeOffsetY);
                lastAttackSpawn = currentFrame;
            }

            for (auto it = attacks.begin(); it != attacks.end(); ) {
                it->timer += deltaTime;

                if (it->type == NORMAL) {
                    it->x += it->dx * deltaTime;
                    it->y += it->dy * deltaTime;
                    if (checkCollision(it->x, it->y, it->width, it->height)) {
                        currentState = GAME_OVER;
                    }
                }
                else if (it->type == BOOMERANG) {
                    it->y += it->dy * deltaTime;
                    if (!it->returning && it->y < -0.4f) { 
                        it->dy *= -1.0f; 
                        it->returning = true; 
                    }
                    if (checkCollision(it->x, it->y, it->width, it->height)) {
                        currentState = GAME_OVER;
                    }
                }
                else if (it->type == LASER) {
                    if (it->timer > 1.0f) { 
                        it->active = true;
                        if (checkCollision(0.0f, it->y, it->width, it->height)) {
                            currentState = GAME_OVER;
                        }
                    }
                    if (it->timer > 1.5f) { 
                        it->active = false;
                    }
                }
                else if (it->type == LONG_LASER) {
                    if (it->timer > 1.0f) {
                        it->active = true;
                        it->r = 1.0f; it->g = 0.8f; it->b = 0.0f; 
                        if (checkCollision(0.0f, it->y, it->width, it->height)) {
                            currentState = GAME_OVER;
                        }
                    }
                    if (it->timer > 4.5f) { 
                        it->active = false;
                    }
                }
                else if (it->type == TILE_DMG) {
                    if (it->timer > 1.2f) { 
                        it->active = true;
                        it->r = 1.0f; it->g = 0.1f; it->b = 0.1f; 
                        if (checkCollision(it->x, it->y, it->width, it->height)) {
                            currentState = GAME_OVER;
                        }
                    }
                    if (it->timer > 2.2f) { 
                        it->active = false;
                    }
                }

                if (it->y > 2.2f || it->y < -2.2f || 
                    (it->type == LASER && it->timer > 1.6f) ||
                    (it->type == LONG_LASER && it->timer > 4.6f) ||
                    (it->type == TILE_DMG && it->timer > 2.3f)) {
                    it = attacks.erase(it);
                } else {
                    ++it;
                }
            }
        }

        if (currentState == MENU) glClearColor(0.1f, 0.15f, 0.25f, 1.0f);
        else if (currentState == SETTINGS) glClearColor(0.25f, 0.1f, 0.1f, 1.0f);
        else glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(shaderProgram);

        if (currentState == MENU) {
            glBindVertexArray(quadVAO);
            // Play
            glUniform2f(offsetLoc, 0.0f, 0.45f); glUniform2f(scaleLoc, 0.8f, 0.3f); glUniform3f(colorLoc, 0.2f, 0.8f, 0.2f);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            // Settings
            glUniform2f(offsetLoc, 0.0f, 0.0f); glUniform2f(scaleLoc, 0.8f, 0.3f); glUniform3f(colorLoc, 0.2f, 0.5f, 0.8f);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            // Quit
            glUniform2f(offsetLoc, 0.0f, -0.45f); glUniform2f(scaleLoc, 0.8f, 0.3f); glUniform3f(colorLoc, 0.8f, 0.2f, 0.2f);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        } 
        else if (currentState == SETTINGS) {
            glBindVertexArray(quadVAO);
            glUniform2f(offsetLoc, 0.0f, 0.0f); glUniform2f(scaleLoc, 0.8f, 0.4f); glUniform3f(colorLoc, 0.8f, 0.4f, 0.2f);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        } 
        else if (currentState == GAME) {
            // Draw Grid
            glBindVertexArray(gridVAO);
            glUniform2f(offsetLoc, 0.0f, 0.0f); glUniform2f(scaleLoc, 1.0f, 1.0f); glUniform3f(colorLoc, 0.3f, 0.3f, 0.3f);
            glDrawArrays(GL_LINES, 0, 16);

            // Draw Player
            glBindVertexArray(quadVAO);
            glUniform2f(offsetLoc, cubeOffsetX, cubeOffsetY); glUniform2f(scaleLoc, 0.3f, 0.3f); glUniform3f(colorLoc, 0.9f, 0.8f, 0.1f);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            // Draw Attacks
            for (const auto& atk : attacks) {
                glUniform2f(offsetLoc, atk.x, atk.y);
                if (atk.type == LASER || atk.type == LONG_LASER) {
                    glUniform2f(scaleLoc, atk.width, atk.active ? atk.height : 0.03f); 
                } 
                else if (atk.type == TILE_DMG) {
                    if (!atk.active) {
                        glUniform2f(scaleLoc, atk.width * 0.7f, atk.height * 0.7f);
                    } else {
                        glUniform2f(scaleLoc, atk.width, atk.height);
                    }
                } 
                else {
                    glUniform2f(scaleLoc, atk.width, atk.height);
                }
                glUniform3f(colorLoc, atk.r, atk.g, atk.b);
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            }
        }
        else if (currentState == GAME_OVER) {
            glBindVertexArray(quadVAO);
            glUniform2f(offsetLoc, 0.0f, 0.0f); glUniform2f(scaleLoc, 1.5f, 1.5f); glUniform3f(colorLoc, 0.6f, 0.0f, 0.0f);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            
            glUniform2f(offsetLoc, cubeOffsetX, cubeOffsetY); glUniform2f(scaleLoc, 0.3f, 0.3f); glUniform3f(colorLoc, 0.0f, 0.0f, 0.0f);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &quadVAO); glDeleteBuffers(1, &quadVBO);
    glDeleteVertexArrays(1, &gridVAO); glDeleteBuffers(1, &gridVBO);
    glDeleteProgram(shaderProgram);
    glfwTerminate();
    return 0;
}