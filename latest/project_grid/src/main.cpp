#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>

// GAME STATE

enum GameState { MENU, SETTINGS, GAME };
GameState currentState = MENU;

float cubeOffsetX = 0.0f;
float cubeOffsetY = 0.0f;

const float GRID_STEP  = 0.4f;
const float GRID_LIMIT = 0.8f;

// SHADER FUNKCE

static unsigned int CompileShader(unsigned int type, const std::string& source)
{
    unsigned int id = glCreateShader(type);
    const char* src = source.c_str();

    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);

    int result;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);

    if (result == GL_FALSE)
    {
        int length;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
        char* message = (char*)alloca(length * sizeof(char));
        glGetShaderInfoLog(id, length, &length, message);

        std::cout << "Failed to compile "
                  << (type == GL_VERTEX_SHADER ? "vertex" : "fragment")
                  << " shader!\n";
        std::cout << message << std::endl;

        glDeleteShader(id);
        return 0;
    }

    return id;
}

static unsigned int CreateShader(const std::string& vertexShader,
                                 const std::string& fragmentShader)
{
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

// SHADER ZDROJÁKY
//R = raw string literal, nemusis vypisovat \n atd. proste vsecko co je mezi () tak vypise tak jak je. (proto neni tenhle comment u toho XD)
std::string vertexShaderSource = R"(#version 330 core 
layout (location = 0) in vec3 aPos;

uniform vec2 offset;
uniform vec2 scale;
uniform vec3 color;

out vec3 fragColor;

void main()
{
    gl_Position = vec4(
        (aPos.x * scale.x) + offset.x,
        (aPos.y * scale.y) + offset.y,
        aPos.z,
        1.0);

    fragColor = color;
})";

std::string fragmentShaderSource = R"(#version 330 core
in vec3 fragColor;
out vec4 FragColor;

void main()
{
    FragColor = vec4(fragColor, 1.0);
})";

// INPUTS

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);

        int width, height;
        glfwGetWindowSize(window, &width, &height);

        float x = (mouseX / width) * 2.0f - 1.0f;
        float y = -((mouseY / height) * 2.0f - 1.0f);

        if (currentState == MENU)
        {
            if (x > -0.4f && x < 0.4f && y > 0.15f && y < 0.45f)
            {
                currentState = GAME;
                cubeOffsetX = 0.0f;
                cubeOffsetY = 0.0f;
            }
            else if (x > -0.4f && x < 0.4f && y > -0.45f && y < -0.15f)
            {
                currentState = SETTINGS;
            }
        }
        else if (currentState == SETTINGS)
        {
            if (x > -0.4f && x < 0.4f && y > -0.2f && y < 0.2f)
            {
                currentState = MENU;
            }
        }
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        if (currentState == GAME)
        {
            if (key == GLFW_KEY_RIGHT && cubeOffsetX < GRID_LIMIT - 0.01f)
                cubeOffsetX += GRID_STEP;

            if (key == GLFW_KEY_LEFT && cubeOffsetX > -GRID_LIMIT + 0.01f)
                cubeOffsetX -= GRID_STEP;

            if (key == GLFW_KEY_UP && cubeOffsetY < GRID_LIMIT - 0.01f)
                cubeOffsetY += GRID_STEP;

            if (key == GLFW_KEY_DOWN && cubeOffsetY > -GRID_LIMIT + 0.01f)
                cubeOffsetY -= GRID_STEP;
        }

        if (key == GLFW_KEY_ESCAPE)
            currentState = MENU;
    }
}

// MAIN

int main()
{
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(600, 600, "5x5 Grid Game", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK)
        return -1;

    unsigned int shaderProgram = CreateShader(vertexShaderSource, fragmentShaderSource);

    // Quad
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

    // Grid
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

    while (!glfwWindowShouldClose(window))
    {
        if (currentState == MENU)
            glClearColor(0.1f, 0.15f, 0.25f, 1.0f);
        else if (currentState == SETTINGS)
            glClearColor(0.25f, 0.1f, 0.1f, 1.0f);
        else
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(shaderProgram);

        if (currentState == MENU)
        {
            glBindVertexArray(quadVAO);

            glUniform2f(offsetLoc, 0.0f, 0.3f);
            glUniform2f(scaleLoc, 0.8f, 0.3f);
            glUniform3f(colorLoc, 0.2f, 0.8f, 0.2f);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            glUniform2f(offsetLoc, 0.0f, -0.3f);
            glUniform2f(scaleLoc, 0.8f, 0.3f);
            glUniform3f(colorLoc, 0.2f, 0.5f, 0.8f);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }
        else if (currentState == SETTINGS)
        {
            glBindVertexArray(quadVAO);

            glUniform2f(offsetLoc, 0.0f, 0.0f);
            glUniform2f(scaleLoc, 0.8f, 0.4f);
            glUniform3f(colorLoc, 0.8f, 0.4f, 0.2f);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }
        else if (currentState == GAME)
        {
            glBindVertexArray(gridVAO);

            glUniform2f(offsetLoc, 0.0f, 0.0f);
            glUniform2f(scaleLoc, 1.0f, 1.0f);
            glUniform3f(colorLoc, 0.3f, 0.3f, 0.3f);
            glDrawArrays(GL_LINES, 0, 16);

            glBindVertexArray(quadVAO);

            glUniform2f(offsetLoc, cubeOffsetX, cubeOffsetY);
            glUniform2f(scaleLoc, 0.3f, 0.3f);
            glUniform3f(colorLoc, 0.9f, 0.8f, 0.1f);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);
    glDeleteVertexArrays(1, &gridVAO);
    glDeleteBuffers(1, &gridVBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}