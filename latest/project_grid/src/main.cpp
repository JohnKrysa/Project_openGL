#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <algorithm>


//Napsal jsem gemini at to udela lehce prehlednejsi pomoci commentu... do ruznejch souboru to jeste roztridim pozdeji. (je to jen at je to zatim prehledne trochu :D)


// ============================================================================
// 1. GAME STATES & ENUMS
// ============================================================================
enum GameState { MENU, SETTINGS, GAME, GAME_OVER };
GameState currentState = MENU;

enum AttackType { NORMAL, LASER, BOOMERANG, LONG_LASER, TILE_DMG, MOVING_LASER };

// ============================================================================
// 2. PLAYER & GRID SETTINGS
// ============================================================================
// Player positions
float cubeOffsetX = 0.0f;
float cubeOffsetY = 0.0f;
float targetOffsetX = 0.0f;
float targetOffsetY = 0.0f;

// Grid configuration
int gridCells = 5; 
std::vector<float> gridCoords = {-0.8f, -0.4f, 0.0f, 0.4f, 0.8f};
float gridLimit = 0.8f;
float gridStep = 0.4f;

// Dynamically recalculates grid lanes based on the cell count
void SetGridSize(int size) {
    gridCells = size;
    gridCoords.clear();
    float start = -0.8f;
    float step = 1.6f / (size - 1);
    for (int i = 0; i < size; ++i) {
        gridCoords.push_back(start + (i * step));
    }
    gridLimit = 0.8f;
    gridStep = step;
}

// ============================================================================
// 3. ATTACK SYSTEM
// ============================================================================
struct Attack {
    AttackType type;
    float x, y;
    float dx, dy;
    float r, g, b;
    float timer = 0.0f;
    float distanceTravelled = 0.0f;
    bool returning = false;
    bool active = true;
    float width = 0.3f;
    float height = 0.3f;
};
std::vector<Attack> attacks;

// Spawn timers and difficulty scaling
float gameTime = 0.0f;
float lastAttackSpawn = 0.0f;
float currentSpawnDelay = 1.2f;
float globalSpeedMultiplier = 1.0f;

// ============================================================================
// 4. WINDOW & GRAPHICS CONFIGURATION
// ============================================================================
bool isFullscreen = false;
int savedWidth = 900, savedHeight = 900;
int savedX = 100, savedY = 100;

unsigned int gridVAO, gridVBO;

// Rebuilds the grid lines to match the current grid size
void UpdateGridVAO() {
    std::vector<float> gridVertices;
    for (float x : gridCoords) {
        gridVertices.push_back(x); gridVertices.push_back(-1.0f); gridVertices.push_back(0.0f);
        gridVertices.push_back(x); gridVertices.push_back(1.0f); gridVertices.push_back(0.0f);
    }
    for (float y : gridCoords) {
        gridVertices.push_back(-1.0f); gridVertices.push_back(y); gridVertices.push_back(0.0f);
        gridVertices.push_back(1.0f); gridVertices.push_back(y); gridVertices.push_back(0.0f);
    }

    glBindVertexArray(gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    glBufferData(GL_ARRAY_BUFFER, gridVertices.size() * sizeof(float), gridVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
}

// ============================================================================
// 5. CORE GAMEPLAY FUNCTIONS
// ============================================================================

// Resets game variables when starting a new round
void ResetGame(GLFWwindow* window) {
    cubeOffsetX = 0.0f;
    cubeOffsetY = 0.0f;
    targetOffsetX = 0.0f;
    targetOffsetY = 0.0f;
    attacks.clear();
    gameTime = 0.0f;
    lastAttackSpawn = 0.0f;
    currentSpawnDelay = 1.2f;
    globalSpeedMultiplier = 1.0f;
    glfwSetWindowTitle(window, "Grid Dodge");
}

// Spawns a randomized hazard from a random screen edge
void SpawnAttack(float playerX, float playerY) { //work in progress
    int attackType = rand() % 16;
    float speed = 1.2f * globalSpeedMultiplier;
    float r = 0.9f, g = 0.1f, b = 0.1f; 

    int edge = rand() % 4; 
    float spawnX = 0.0f, spawnY = 0.0f, dx = 0.0f, dy = 0.0f;
    float randPos = gridCoords[rand() % gridCells];

    // Trajectory mapping
    if (edge == 0) { spawnX = randPos; spawnY = 1.4f; dy = -speed; }
    else if (edge == 1) { spawnX = randPos; spawnY = -1.4f; dy = speed; }
    else if (edge == 2) { spawnX = -1.4f; spawnY = randPos; dx = speed; }
    else if (edge == 3) { spawnX = 1.4f; spawnY = randPos; dx = -speed; }

    bool isHorizontal = (edge == 0 || edge == 1); 

    // Dice rolls to pick attack variety
    if (attackType < 4) { // NORMAL
        attacks.push_back({NORMAL, spawnX, spawnY, dx, dy, r, g, b, 0.0f, 0.0f, false, true, 0.3f, 0.3f});
    } 
    else if (attackType < 6) { // LASER
        if (isHorizontal) attacks.push_back({LASER, 0.0f, gridCoords[rand() % gridCells], 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, false, false, 2.8f, 0.15f});
        else attacks.push_back({LASER, gridCoords[rand() % gridCells], 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, false, false, 0.15f, 2.8f});
    } 
    else if (attackType < 8) { // BOOMERANG
        attacks.push_back({BOOMERANG, spawnX, spawnY, dx * 1.2f, dy * 1.2f, 0.2f, 0.8f, 0.9f, 0.0f, 0.0f, false, true, 0.3f, 0.3f});
    }
    else if (attackType < 10) { // LONG_LASER
        if (isHorizontal) attacks.push_back({LONG_LASER, 0.0f, gridCoords[rand() % gridCells], 0.0f, 0.0f, 0.5f, 0.4f, 0.0f, 0.0f, 0.0f, false, false, 2.8f, 0.15f});
        else attacks.push_back({LONG_LASER, gridCoords[rand() % gridCells], 0.0f, 0.0f, 0.0f, 0.5f, 0.4f, 0.0f, 0.0f, 0.0f, false, false, 0.15f, 2.8f});
    }
    else if (attackType < 12) { // TILE_DMG
        float rx = gridCoords[rand() % gridCells];
        float ry = gridCoords[rand() % gridCells];
        bool big = (rand() % 2 == 0);
        float size = big ? 0.8f : 0.4f;
        attacks.push_back({TILE_DMG, rx, ry, 0.0f, 0.0f, 0.4f, 0.1f, 0.1f, 0.0f, 0.0f, false, false, size, size});
    }
    else if (attackType < 14) { // MOVING_LASER
        if (isHorizontal) attacks.push_back({MOVING_LASER, 0.0f, spawnY, 0.0f, dy * 0.25f, 0.8f, 0.5f, 0.2f, 0.0f, 0.0f, false, true, 2.8f, 0.15f});
        else attacks.push_back({MOVING_LASER, spawnX, 0.0f, dx * 0.25f, 0.0f, 0.8f, 0.5f, 0.2f, 0.0f, 0.0f, false, true, 0.15f, 2.8f});
    }
    else { // TARGETED SHOT testuju pouze (WIP)
        float tx = playerX - spawnX;
        float ty = playerY - spawnY;
        float len = std::sqrt(tx*tx + ty*ty);
        if (len != 0) { tx /= len; ty /= len; }
        attacks.push_back({NORMAL, spawnX, spawnY, tx * speed * 1.5f, ty * speed * 1.5f, 0.8f, 0.1f, 0.8f, 0.0f, 0.0f, false, true, 0.3f, 0.3f});
    }
}

// Collision Detection
bool checkCollision(float ax, float ay, float aw, float ah) {
    return std::abs(cubeOffsetX - ax) < (0.15f + aw / 2.0f) && 
           std::abs(cubeOffsetY - ay) < (0.15f + ah / 2.0f);
}

// ============================================================================
// 6. SHADER COMPILATION
// ============================================================================
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

// Custom simple shaders for colored squares and lines
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

// ============================================================================
// 7. GLFW CALLBACKS & INPUTS
// ============================================================================
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);
        int width, height;
        glfwGetWindowSize(window, &width, &height);

        float x = ((mouseX / width) * 2.0f - 1.0f) / 0.7f;
        float y = (-((mouseY / height) * 2.0f - 1.0f)) / 0.7f;

        if (currentState == MENU) {
            if (x > -0.4f && x < 0.4f && y > 0.3f && y < 0.6f) {
                ResetGame(window);
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

void ToggleFullscreen(GLFWwindow* window) { //work in progress
    if (!isFullscreen) {
        glfwGetWindowPos(window, &savedX, &savedY);
        glfwGetWindowSize(window, &savedWidth, &savedHeight);
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    } else {
        glfwSetWindowMonitor(window, NULL, savedX, savedY, savedWidth, savedHeight, 0);
    }
    isFullscreen = !isFullscreen;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        // Game Movement
        if (currentState == GAME) {
            if (key == GLFW_KEY_RIGHT && targetOffsetX < gridLimit - 0.01f) targetOffsetX += gridStep;
            if (key == GLFW_KEY_LEFT && targetOffsetX > -gridLimit + 0.01f) targetOffsetX -= gridStep;
            if (key == GLFW_KEY_UP && targetOffsetY < gridLimit - 0.01f) targetOffsetY += gridStep;
            if (key == GLFW_KEY_DOWN && targetOffsetY > -gridLimit + 0.01f) targetOffsetY -= gridStep;
        }

        if (currentState == GAME_OVER && key == GLFW_KEY_R) {
            ResetGame(window);
            currentState = GAME;
        }

        if (key == GLFW_KEY_ESCAPE) currentState = MENU;

        // Settings Toggles
        if (currentState == SETTINGS && key == GLFW_KEY_G) {
            int nextSize = gridCells + 2;
            if (nextSize > 15) nextSize = 5;
            SetGridSize(nextSize);
            UpdateGridVAO();
        }

        // Screen Resolution Shortcuts
        if (key == GLFW_KEY_F1) { if (isFullscreen) ToggleFullscreen(window); glfwSetWindowSize(window, 600, 600); }
        if (key == GLFW_KEY_F2) { if (isFullscreen) ToggleFullscreen(window); glfwSetWindowSize(window, 900, 900); }
        if (key == GLFW_KEY_F3) { if (isFullscreen) ToggleFullscreen(window); glfwSetWindowSize(window, 1200, 900); }
        if (key == GLFW_KEY_F4) { ToggleFullscreen(window); }
    }
}

// ============================================================================
// 8. MAIN FUNCTION & EXECUTION LOOP
// ============================================================================
int main() {
    srand(time(NULL)); 

    // GLFW & Window Init
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(900, 900, "Grid Dodge", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
    if (glewInit() != GLEW_OK) return -1;

    unsigned int shaderProgram = CreateShader(vertexShaderSource, fragmentShaderSource);

    // Quad Buffer Generation
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

    glGenVertexArrays(1, &gridVAO);
    glGenBuffers(1, &gridVBO);
    UpdateGridVAO();

    // Attach Callbacks
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetKeyCallback(window, key_callback);

    int offsetLoc = glGetUniformLocation(shaderProgram, "offset");
    int scaleLoc  = glGetUniformLocation(shaderProgram, "scale");
    int colorLoc  = glGetUniformLocation(shaderProgram, "color");

    float lastFrame = 0.0f;

    // --- GAME LOOP ---
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        if (currentState == GAME) {
            gameTime += deltaTime;
            
            // Linear interpolation
            cubeOffsetX += (targetOffsetX - cubeOffsetX) * 15.0f * deltaTime;
            cubeOffsetY += (targetOffsetY - cubeOffsetY) * 15.0f * deltaTime;
            
            // Difficulty scaling
            globalSpeedMultiplier = 1.0f + (gameTime * 0.05f); 
            currentSpawnDelay = std::max(0.3f, 1.2f - (gameTime * 0.02f));

            // Enemy spawner ticker
            if (currentFrame - lastAttackSpawn > currentSpawnDelay) {
                SpawnAttack(targetOffsetX, targetOffsetY); 
                lastAttackSpawn = currentFrame;
            }

            // Attack processing loop -- docasne nez bude level editor
            for (auto it = attacks.begin(); it != attacks.end(); ) {
                it->timer += deltaTime;
                
                float moveDist = std::sqrt(it->dx * it->dx + it->dy * it->dy) * deltaTime;
                it->distanceTravelled += moveDist;

                // Physics logic per attack type
                if (it->type == NORMAL) {
                    it->x += it->dx * deltaTime;
                    it->y += it->dy * deltaTime;
                    if (checkCollision(it->x, it->y, it->width, it->height)) currentState = GAME_OVER;
                }
                else if (it->type == BOOMERANG) {
                    it->x += it->dx * deltaTime;
                    it->y += it->dy * deltaTime;
                    if (!it->returning && it->distanceTravelled > 1.8f) { 
                        it->dx *= -1.0f; 
                        it->dy *= -1.0f; 
                        it->returning = true; 
                    }
                    if (checkCollision(it->x, it->y, it->width, it->height)) currentState = GAME_OVER;
                }
                else if (it->type == LASER) {
                    if (it->timer > 1.0f) { 
                        it->active = true;
                        if (checkCollision(it->x, it->y, it->width, it->height)) currentState = GAME_OVER; 
                    }
                    if (it->timer > 1.5f) it->active = false;
                }
                else if (it->type == LONG_LASER) {
                    if (it->timer > 1.0f) {
                        it->active = true;
                        it->r = 1.0f; it->g = 0.8f; it->b = 0.0f; 
                        if (checkCollision(it->x, it->y, it->width, it->height)) currentState = GAME_OVER;
                    }
                    if (it->timer > 4.5f) it->active = false;
                }
                else if (it->type == TILE_DMG) {
                    if (it->timer > 1.2f) { 
                        it->active = true;
                        it->r = 1.0f; it->g = 0.1f; it->b = 0.1f; 
                        if (checkCollision(it->x, it->y, it->width, it->height)) currentState = GAME_OVER;
                    }
                    if (it->timer > 2.2f) it->active = false;
                }
                else if (it->type == MOVING_LASER) {
                    it->x += it->dx * deltaTime;
                    it->y += it->dy * deltaTime;
                    if (checkCollision(it->x, it->y, it->width, it->height)) currentState = GAME_OVER;
                }

                // Garbage Collection (Off-screen cleaning and timed removal)
                if (it->y > 2.2f || it->y < -2.2f || it->x > 2.2f || it->x < -2.2f || 
                    (it->type == LASER && it->timer > 1.6f) ||
                    (it->type == LONG_LASER && it->timer > 4.6f) ||
                    (it->type == TILE_DMG && it->timer > 2.3f) ||
                    (it->type == MOVING_LASER && it->distanceTravelled >= (0.6f + (2.0f * gridStep)))) {
                    it = attacks.erase(it);
                } else {
                    ++it;
                }
            }

            if (currentState == GAME_OVER) { 
                std::cout << "Game Over! Your Score: " << (int)gameTime << std::endl;
                std::string title = "Grid Dodge - Game Over! Score: " + std::to_string((int)gameTime);
                glfwSetWindowTitle(window, title.c_str());
            }
        }

        // Color backgrounds for different game states
        if (currentState == MENU) glClearColor(0.1f, 0.15f, 0.25f, 1.0f);
        else if (currentState == SETTINGS) glClearColor(0.25f, 0.1f, 0.1f, 1.0f);
        else glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(shaderProgram);

        // --- DRAWING LOOPS ---
        if (currentState == MENU) {
            glBindVertexArray(quadVAO);
            glUniform2f(offsetLoc, 0.0f, 0.45f); glUniform2f(scaleLoc, 0.8f, 0.3f); glUniform3f(colorLoc, 0.2f, 0.8f, 0.2f);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            glUniform2f(offsetLoc, 0.0f, 0.0f); glUniform2f(scaleLoc, 0.8f, 0.3f); glUniform3f(colorLoc, 0.2f, 0.5f, 0.8f);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            glUniform2f(offsetLoc, 0.0f, -0.45f); glUniform2f(scaleLoc, 0.8f, 0.3f); glUniform3f(colorLoc, 0.8f, 0.2f, 0.2f);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        } 
        else if (currentState == SETTINGS) {
            glBindVertexArray(quadVAO);
            glUniform2f(offsetLoc, 0.0f, 0.0f); glUniform2f(scaleLoc, 0.8f, 0.4f); glUniform3f(colorLoc, 0.8f, 0.4f, 0.2f);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        } 
        else if (currentState == GAME) {
            // Draw lines
            glBindVertexArray(gridVAO);
            glUniform2f(offsetLoc, 0.0f, 0.0f); glUniform2f(scaleLoc, 1.0f, 1.0f); glUniform3f(colorLoc, 0.3f, 0.3f, 0.3f);
            glDrawArrays(GL_LINES, 0, gridCoords.size() * 4);

            // Draw player
            glBindVertexArray(quadVAO);
            glUniform2f(offsetLoc, cubeOffsetX, cubeOffsetY); glUniform2f(scaleLoc, 0.3f, 0.3f); glUniform3f(colorLoc, 0.9f, 0.8f, 0.1f);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            // Draw current active hazards
            for (const auto& atk : attacks) {
                glUniform2f(offsetLoc, atk.x, atk.y);
                if (atk.type == LASER || atk.type == LONG_LASER) {
                    if (atk.width > atk.height) { // Horizontal
                        glUniform2f(scaleLoc, atk.width, atk.active ? atk.height : 0.03f);
                    } else { // Vertical
                        glUniform2f(scaleLoc, atk.active ? atk.width : 0.03f, atk.height); 
                    }
                } 
                else if (atk.type == TILE_DMG) {
                    if (!atk.active) glUniform2f(scaleLoc, atk.width * 0.7f, atk.height * 0.7f);
                    else glUniform2f(scaleLoc, atk.width, atk.height);
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

    // Cleanup
    glDeleteVertexArrays(1, &quadVAO); glDeleteBuffers(1, &quadVBO);
    glDeleteVertexArrays(1, &gridVAO); glDeleteBuffers(1, &gridVBO);
    glDeleteProgram(shaderProgram);
    glfwTerminate();
    return 0;
}