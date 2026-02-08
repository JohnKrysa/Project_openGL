#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include <cstdlib>
#include <ctime>
#include <cmath>

// shadery
const char* vertexShaderSource = R"glsl(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    uniform vec2 offset; 
    uniform float scale; 
    void main() {
        gl_Position = vec4((aPos.x * scale) + offset.x, (aPos.y * scale) + offset.y, aPos.z, 1.0);
    }
)glsl";

const char* fragmentShaderSource = R"glsl(
    #version 330 core
    out vec4 FragColor;
    uniform vec4 color; // Changed to vec4 for Alpha support
    void main() {
        FragColor = color;
    }
)glsl";

const int GRID_SIZE = 5;
const float CELL_SIZE = 2.0f / GRID_SIZE;

enum GameState { MENU, PLAYING };
GameState currentState = MENU;

int targetPlayerX = 2, targetPlayerY = 2;
float visualPlayerX = 2.0f, visualPlayerY = 2.0f;
const float MOVE_SPEED = 15.0f;

// danger Variables
int hazardX = -1, hazardY = -1;
float hazardMoveTimer = 0.0f;
float hazardSpeed = 1.2f;     
float warningTimer = 0.0f;
const float WARNING_DURATION = 0.5f; 
bool isHazardWarning = false;

float gameTime = 60.0f;
const float GRACE_DURATION = 2.0f;
float graceTimer = 0.0f;

void resetGame() {
    targetPlayerX = 2; targetPlayerY = 2;
    visualPlayerX = 2.0f; visualPlayerY = 2.0f;
    hazardX = -1; hazardY = -1;
    gameTime = 60.0f;
    hazardMoveTimer = 0.0f;
    graceTimer = 0.0f;
    isHazardWarning = false;
}

float lerp(float a, float b, float f) { return a + f * (b - a); }

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        if (currentState == MENU && key == GLFW_KEY_ENTER) {
            resetGame();
            currentState = PLAYING;
        } 
        else if (currentState == PLAYING) {
            if (key == GLFW_KEY_RIGHT && targetPlayerX < GRID_SIZE - 1) targetPlayerX++;
            if (key == GLFW_KEY_LEFT  && targetPlayerX > 0)             targetPlayerX--;
            if (key == GLFW_KEY_UP    && targetPlayerY < GRID_SIZE - 1) targetPlayerY++;
            if (key == GLFW_KEY_DOWN  && targetPlayerY > 0)             targetPlayerY--;
        }
    }
}

unsigned int compileShader(unsigned int type, const char* source) {
    unsigned int id = glCreateShader(type);
    glShaderSource(id, 1, &source, nullptr);
    glCompileShader(id);
    return id;
}

int main() {
    std::srand(static_cast<unsigned int>(std::time(0)));
    if (!glfwInit()) return -1;
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(600, 600, "Dodge Grid", NULL, NULL);
    glfwMakeContextCurrent(window);
    glewInit();
    glfwSetKeyCallback(window, key_callback);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    unsigned int vs = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    unsigned int fs = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vs);
    glAttachShader(shaderProgram, fs);
    glLinkProgram(shaderProgram);
    glUseProgram(shaderProgram);

    float half = (CELL_SIZE * 0.8f) / 2.0f;
    float squareVertices[] = { half, -half, 0.0f, half, half, 0.0f, -half, -half, 0.0f, -half, half, 0.0f };
    unsigned int VAO_Square, VBO_Square;
    glGenVertexArrays(1, &VAO_Square); glGenBuffers(1, &VBO_Square);
    glBindVertexArray(VAO_Square); glBindBuffer(GL_ARRAY_BUFFER, VBO_Square);
    glBufferData(GL_ARRAY_BUFFER, sizeof(squareVertices), squareVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    float triVertices[] = { -0.2f, -0.2f, 0.0f, 0.2f, 0.0f, 0.0f, -0.2f, 0.2f, 0.0f };
    unsigned int VAO_Tri, VBO_Tri;
    glGenVertexArrays(1, &VAO_Tri); glGenBuffers(1, &VBO_Tri);
    glBindVertexArray(VAO_Tri); glBindBuffer(GL_ARRAY_BUFFER, VBO_Tri);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triVertices), triVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    int offsetLoc = glGetUniformLocation(shaderProgram, "offset");
    int colorLoc  = glGetUniformLocation(shaderProgram, "color");
    int scaleLoc  = glGetUniformLocation(shaderProgram, "scale");

    float lastFrame = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        if (currentState == MENU) {
            float pulse = 1.0f + std::sin(currentFrame * 3.0f) * 0.1f;
            glUniform2f(offsetLoc, 0.0f, 0.0f);
            glUniform4f(colorLoc, 0.0f, 1.0f, 0.5f, 1.0f); 
            glUniform1f(scaleLoc, pulse);
            glBindVertexArray(VAO_Tri);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }
        else if (currentState == PLAYING) {

            visualPlayerX = lerp(visualPlayerX, (float)targetPlayerX, deltaTime * MOVE_SPEED);
            visualPlayerY = lerp(visualPlayerY, (float)targetPlayerY, deltaTime * MOVE_SPEED);
            gameTime -= deltaTime;
            graceTimer += deltaTime;

            if (graceTimer > GRACE_DURATION) {
                hazardMoveTimer += deltaTime;
                
                if (hazardMoveTimer > hazardSpeed) {
                    hazardMoveTimer = 0.0f;
                    hazardX = targetPlayerX;
                    hazardY = targetPlayerY;
                    isHazardWarning = true;
                    warningTimer = 0.0f;
                }

                if (isHazardWarning) {
                    warningTimer += deltaTime;
                    if (warningTimer >= WARNING_DURATION) {
                        isHazardWarning = false; // It is now "Active"
                    }
                }

                if (!isHazardWarning && hazardX != -1) {
                    if (targetPlayerX == hazardX && targetPlayerY == hazardY) {
                        currentState = MENU;
                    }
                }
            }

            if (gameTime <= 0.0f) currentState = MENU;

            glBindVertexArray(VAO_Square);
            
            float pX = -1.0f + (CELL_SIZE / 2.0f) + (visualPlayerX * CELL_SIZE);
            float pY = -1.0f + (CELL_SIZE / 2.0f) + (visualPlayerY * CELL_SIZE);
            float breath = 1.0f + std::sin(currentFrame * 6.0f) * 0.05f;
            glUniform2f(offsetLoc, pX, pY);
            glUniform4f(colorLoc, 0.0f, 0.7f, 1.0f, 1.0f);
            glUniform1f(scaleLoc, breath);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            if (hazardX != -1) {
                float hX = -1.0f + (CELL_SIZE / 2.0f) + (hazardX * CELL_SIZE);
                float hY = -1.0f + (CELL_SIZE / 2.0f) + (hazardY * CELL_SIZE);
                glUniform2f(offsetLoc, hX, hY);
                glUniform1f(scaleLoc, 1.0f);
                
                if (isHazardWarning) {
                    glUniform4f(colorLoc, 1.0f, 0.0f, 0.0f, 0.3f); 
                } else {
                    glUniform4f(colorLoc, 1.0f, 0.0f, 0.0f, 1.0f);
                }
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            }
        }
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwTerminate();
    return 0;
}