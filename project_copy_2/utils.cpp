/**
 * @file utils.cpp
 * @brief Utility rendering and text helper functions.
 */

#include "common.h"

std::string vertexShaderSource = R"(#version 330 core
layout (location = 0) in vec3 aPos;
uniform vec2 offset;
uniform vec2 scale;
uniform vec2 viewOffset;
uniform vec2 viewScale;
uniform float aspect;
void main() {
    vec2 pos = ((aPos.xy * scale) + offset) * viewScale + viewOffset;
    gl_Position = vec4(pos.x / aspect * 0.7, pos.y * 0.7, 0.0, 1.0);
})";

std::string fragmentShaderSource = R"(#version 330 core
uniform vec4 color;
out vec4 FragColor;
void main() { FragColor = color; })";

static unsigned int CompileShader(unsigned int type, const std::string& source) {
    unsigned int id = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);
    int result;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE) {
        int length; glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
        char* message = (char*)alloca(length * sizeof(char));
        glGetShaderInfoLog(id, length, &length, message);
        glDeleteShader(id); return 0;
    }
    return id;
}

unsigned int CreateShader(const std::string& vs, const std::string& fs) {
    unsigned int program = glCreateProgram();
    unsigned int v = CompileShader(GL_VERTEX_SHADER, vs);
    unsigned int f = CompileShader(GL_FRAGMENT_SHADER, fs);
    glAttachShader(program, v); glAttachShader(program, f);
    glLinkProgram(program); glValidateProgram(program);
    glDeleteShader(v); glDeleteShader(f);
    return program;
}

void AddLine(std::vector<float>& v, float x1, float y1, float x2, float y2,
             float ox, float oy, float s) {
    v.push_back(ox+x1*s); v.push_back(oy+y1*s); v.push_back(0.0f);
    v.push_back(ox+x2*s); v.push_back(oy+y2*s); v.push_back(0.0f);
}

float TextWidth(const std::string& str, float scale) {
    if (str.empty()) return 0.0f;
    return ((float)str.size() * 1.6f - 0.6f) * scale;
}

float CenterX(const std::string& str, float scale, float centerX) {
    return centerX - TextWidth(str, scale) / 2.0f;
}

std::vector<float> GenerateText(const std::string& str, float startX, float startY, float scale) {
    std::vector<float> pts;
    float x = startX;
    for (char c : str) {
        switch (c) {
        case 'A': AddLine(pts,0,0,0,2,x,startY,scale); AddLine(pts,0,2,1,2,x,startY,scale); AddLine(pts,1,2,1,0,x,startY,scale); AddLine(pts,0,1,1,1,x,startY,scale); break;
        case 'B': AddLine(pts,0,0,0,2,x,startY,scale); AddLine(pts,0,2,0.8f,2,x,startY,scale); AddLine(pts,0.8f,2,1,1.5f,x,startY,scale); AddLine(pts,1,1.5f,0.8f,1,x,startY,scale); AddLine(pts,0.8f,1,0,1,x,startY,scale); AddLine(pts,0.8f,1,1,0.5f,x,startY,scale); AddLine(pts,1,0.5f,0.8f,0,x,startY,scale); AddLine(pts,0.8f,0,0,0,x,startY,scale); break;
        case 'C': AddLine(pts,1,2,0,2,x,startY,scale); AddLine(pts,0,2,0,0,x,startY,scale); AddLine(pts,0,0,1,0,x,startY,scale); break;
        case 'D': AddLine(pts,0,0,0,2,x,startY,scale); AddLine(pts,0,2,0.8f,2,x,startY,scale); AddLine(pts,0.8f,2,1,1,x,startY,scale); AddLine(pts,1,1,0.8f,0,x,startY,scale); AddLine(pts,0.8f,0,0,0,x,startY,scale); break;
        case 'E': AddLine(pts,1,2,0,2,x,startY,scale); AddLine(pts,0,2,0,0,x,startY,scale); AddLine(pts,0,0,1,0,x,startY,scale); AddLine(pts,0,1,0.8f,1,x,startY,scale); break;
        case 'F': AddLine(pts,1,2,0,2,x,startY,scale); AddLine(pts,0,2,0,0,x,startY,scale); AddLine(pts,0,1,0.8f,1,x,startY,scale); break;
        case 'G': AddLine(pts,1,2,0,2,x,startY,scale); AddLine(pts,0,2,0,0,x,startY,scale); AddLine(pts,0,0,1,0,x,startY,scale); AddLine(pts,1,0,1,1,x,startY,scale); AddLine(pts,0.5f,1,1,1,x,startY,scale); break;
        case 'H': AddLine(pts,0,0,0,2,x,startY,scale); AddLine(pts,1,0,1,2,x,startY,scale); AddLine(pts,0,1,1,1,x,startY,scale); break;
        case 'I': AddLine(pts,0.5f,0,0.5f,2,x,startY,scale); AddLine(pts,0.2f,0,0.8f,0,x,startY,scale); AddLine(pts,0.2f,2,0.8f,2,x,startY,scale); break;
        case 'J': AddLine(pts,0.8f,2,0.8f,0.3f,x,startY,scale); AddLine(pts,0.8f,0.3f,0.5f,0,x,startY,scale); AddLine(pts,0.5f,0,0,0.3f,x,startY,scale); break;
        case 'K': AddLine(pts,0,0,0,2,x,startY,scale); AddLine(pts,0,1,1,2,x,startY,scale); AddLine(pts,0,1,1,0,x,startY,scale); break;
        case 'L': AddLine(pts,0,2,0,0,x,startY,scale); AddLine(pts,0,0,1,0,x,startY,scale); break;
        case 'M': AddLine(pts,0,0,0,2,x,startY,scale); AddLine(pts,0,2,0.5f,1,x,startY,scale); AddLine(pts,0.5f,1,1,2,x,startY,scale); AddLine(pts,1,2,1,0,x,startY,scale); break;
        case 'N': AddLine(pts,0,0,0,2,x,startY,scale); AddLine(pts,0,2,1,0,x,startY,scale); AddLine(pts,1,0,1,2,x,startY,scale); break;
        case 'O': AddLine(pts,0,0,0,2,x,startY,scale); AddLine(pts,0,2,1,2,x,startY,scale); AddLine(pts,1,2,1,0,x,startY,scale); AddLine(pts,1,0,0,0,x,startY,scale); break;
        case 'P': AddLine(pts,0,0,0,2,x,startY,scale); AddLine(pts,0,2,1,2,x,startY,scale); AddLine(pts,1,2,1,1,x,startY,scale); AddLine(pts,1,1,0,1,x,startY,scale); break;
        case 'Q': AddLine(pts,0,0,0,2,x,startY,scale); AddLine(pts,0,2,1,2,x,startY,scale); AddLine(pts,1,2,1,0,x,startY,scale); AddLine(pts,1,0,0,0,x,startY,scale); AddLine(pts,0.5f,0.5f,1,-0.2f,x,startY,scale); break;
        case 'R': AddLine(pts,0,0,0,2,x,startY,scale); AddLine(pts,0,2,1,2,x,startY,scale); AddLine(pts,1,2,1,1,x,startY,scale); AddLine(pts,1,1,0,1,x,startY,scale); AddLine(pts,0,1,1,0,x,startY,scale); break;
        case 'S': AddLine(pts,1,2,0,2,x,startY,scale); AddLine(pts,0,2,0,1,x,startY,scale); AddLine(pts,0,1,1,1,x,startY,scale); AddLine(pts,1,1,1,0,x,startY,scale); AddLine(pts,1,0,0,0,x,startY,scale); break;
        case 'T': AddLine(pts,0,2,1,2,x,startY,scale); AddLine(pts,0.5f,2,0.5f,0,x,startY,scale); break;
        case 'U': AddLine(pts,0,2,0,0,x,startY,scale); AddLine(pts,0,0,1,0,x,startY,scale); AddLine(pts,1,0,1,2,x,startY,scale); break;
        case 'V': AddLine(pts,0,2,0.5f,0,x,startY,scale); AddLine(pts,0.5f,0,1,2,x,startY,scale); break;
        case 'W': AddLine(pts,0,2,0.2f,0,x,startY,scale); AddLine(pts,0.2f,0,0.5f,1,x,startY,scale); AddLine(pts,0.5f,1,0.8f,0,x,startY,scale); AddLine(pts,0.8f,0,1,2,x,startY,scale); break;
        case 'X': AddLine(pts,0,0,1,2,x,startY,scale); AddLine(pts,1,0,0,2,x,startY,scale); break;
        case 'Y': AddLine(pts,0,2,0.5f,1,x,startY,scale); AddLine(pts,1,2,0.5f,1,x,startY,scale); AddLine(pts,0.5f,1,0.5f,0,x,startY,scale); break;
        case 'Z': AddLine(pts,0,2,1,2,x,startY,scale); AddLine(pts,1,2,0,0,x,startY,scale); AddLine(pts,0,0,1,0,x,startY,scale); break;
        case '0': AddLine(pts,0,0,0,2,x,startY,scale); AddLine(pts,0,2,1,2,x,startY,scale); AddLine(pts,1,2,1,0,x,startY,scale); AddLine(pts,1,0,0,0,x,startY,scale); AddLine(pts,0,0,1,2,x,startY,scale); break;
        case '1': AddLine(pts,0.2f,1.5f,0.5f,2,x,startY,scale); AddLine(pts,0.5f,2,0.5f,0,x,startY,scale); AddLine(pts,0.2f,0,0.8f,0,x,startY,scale); break;
        case '2': AddLine(pts,0,2,1,2,x,startY,scale); AddLine(pts,1,2,1,1,x,startY,scale); AddLine(pts,1,1,0,0,x,startY,scale); AddLine(pts,0,0,1,0,x,startY,scale); break;
        case '3': AddLine(pts,0,2,1,2,x,startY,scale); AddLine(pts,1,2,1,0,x,startY,scale); AddLine(pts,1,0,0,0,x,startY,scale); AddLine(pts,0.2f,1,0.9f,1,x,startY,scale); break;
        case '4': AddLine(pts,0.8f,0,0.8f,2,x,startY,scale); AddLine(pts,0.8f,2,0,0.8f,x,startY,scale); AddLine(pts,0,0.8f,1,0.8f,x,startY,scale); break;
        case '5': AddLine(pts,1,2,0,2,x,startY,scale); AddLine(pts,0,2,0,1,x,startY,scale); AddLine(pts,0,1,1,1,x,startY,scale); AddLine(pts,1,1,1,0,x,startY,scale); AddLine(pts,1,0,0,0,x,startY,scale); break;
        case '6': AddLine(pts,1,2,0,2,x,startY,scale); AddLine(pts,0,2,0,0,x,startY,scale); AddLine(pts,0,0,1,0,x,startY,scale); AddLine(pts,1,0,1,1,x,startY,scale); AddLine(pts,1,1,0,1,x,startY,scale); break;
        case '7': AddLine(pts,0,2,1,2,x,startY,scale); AddLine(pts,1,2,0.3f,0,x,startY,scale); break;
        case '8': AddLine(pts,0,0,0,2,x,startY,scale); AddLine(pts,0,2,1,2,x,startY,scale); AddLine(pts,1,2,1,0,x,startY,scale); AddLine(pts,1,0,0,0,x,startY,scale); AddLine(pts,0,1,1,1,x,startY,scale); break;
        case '9': AddLine(pts,0,2,1,2,x,startY,scale); AddLine(pts,1,2,1,0,x,startY,scale); AddLine(pts,0,2,0,1,x,startY,scale); AddLine(pts,0,1,1,1,x,startY,scale); AddLine(pts,1,0,0,0,x,startY,scale); break;
        case ':': AddLine(pts,0.4f,1.4f,0.6f,1.4f,x,startY,scale); AddLine(pts,0.4f,0.6f,0.6f,0.6f,x,startY,scale); break;
        case ' ': break;
        case '.': AddLine(pts,0.3f,0,0.7f,0,x,startY,scale); break;
        case '/': AddLine(pts,0.2f,0,0.8f,2,x,startY,scale); break;
        case ',': AddLine(pts,0.5f,0.3f,0.3f,-0.2f,x,startY,scale); break;
        case '-': AddLine(pts,0.2f,1.0f,0.8f,1.0f,x,startY,scale); break;
        case '+': AddLine(pts,0.5f,0.2f,0.5f,1.8f,x,startY,scale); AddLine(pts,0.1f,1.0f,0.9f,1.0f,x,startY,scale); break;
        default: break;
        }
        x += scale * 1.6f;
    }
    return pts;
}

void DrawDynamicLines(const std::vector<float>& pts) {
    if (pts.empty()) return;
    glBindVertexArray(textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferData(GL_ARRAY_BUFFER, pts.size() * sizeof(float), pts.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glDrawArrays(GL_LINES, 0, (GLsizei)(pts.size() / 3));
}