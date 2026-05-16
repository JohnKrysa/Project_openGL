#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <algorithm>

enum GameState { MENU, SETTINGS, GAME, GAME_OVER, LEVEL_EDITOR };
GameState currentState = MENU;

enum AttackType { NORMAL, LASER, BOOMERANG, LONG_LASER, TILE_DMG, MOVING_LASER };

float cubeOffsetX = 0.0f;
float cubeOffsetY = 0.0f;
float targetOffsetX = 0.0f;
float targetOffsetY = 0.0f;

int gridCells = 5;
std::vector<float> gridCoords = { -0.8f, -0.4f, 0.0f, 0.4f, 0.8f };
float gridLimit = 0.8f;
float gridStep  = 0.4f;

void SetGridSize(int size) {
    gridCells = size;
    gridCoords.clear();
    float start = -0.8f;
    float step  = 1.6f / (size - 1);
    for (int i = 0; i < size; ++i)
        gridCoords.push_back(start + i * step);
    gridLimit = 0.8f;
    gridStep  = step;
}

struct Attack {
    AttackType type;
    float x, y, dx, dy;
    float r, g, b;
    float timer           = 0.0f;
    float distanceTravelled = 0.0f;
    bool  returning       = false;
    bool  active          = true;
    float width           = 0.3f;
    float height          = 0.3f;
};
std::vector<Attack> attacks;

float gameTime            = 0.0f;
float lastAttackSpawn     = 0.0f;
float currentSpawnDelay   = 1.2f;
float globalSpeedMultiplier = 1.0f;
int   score               = 0;

struct LevelEvent {
    AttackType type;
    float triggerTime;
    int   gridCol;
    int   gridRow;
    int   edge; 
};

struct Level {
    std::string            name;
    std::vector<LevelEvent> events;
    float                  duration = 60.0f;
};

Level        currentLevel;
int          editorCursorCol      = 0;
int          editorCursorRow      = 0;
float        editorTime           = 0.0f;
AttackType   editorSelectedType   = NORMAL;
int          editorSelectedEdge   = 0; 
bool         editorPlaying        = false;
float        editorPlayStart      = 0.0f;
size_t       editorNextEvent      = 0;
bool         levelMode            = false; 
bool         isDraggingTimeline   = false; 

bool isFullscreen  = false;
int  savedWidth    = 900, savedHeight = 900;
int  savedX        = 100, savedY      = 100;

unsigned int gridVAO, gridVBO;
unsigned int textVAO, textVBO;

void UpdateGridVAO() {
    std::vector<float> v;
    for (float x : gridCoords) {
        v.push_back(x);  v.push_back(-1.0f); v.push_back(0.0f);
        v.push_back(x);  v.push_back( 1.0f); v.push_back(0.0f);
    }
    for (float y : gridCoords) {
        v.push_back(-1.0f); v.push_back(y); v.push_back(0.0f);
        v.push_back( 1.0f); v.push_back(y); v.push_back(0.0f);
    }
    glBindVertexArray(gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(float), v.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
}

void ResetGame(GLFWwindow* window) {
    cubeOffsetX   = 0.0f; cubeOffsetY   = 0.0f;
    targetOffsetX = 0.0f; targetOffsetY = 0.0f;
    attacks.clear();
    gameTime          = 0.0f;
    lastAttackSpawn   = 0.0f;
    currentSpawnDelay = 1.2f;
    globalSpeedMultiplier = 1.0f;
    score             = 0;
    editorNextEvent   = 0;
    glfwSetWindowTitle(window, "Grid Dodge");
}

void GetAttackColor(AttackType t, float& r, float& g, float& b) {
    switch (t) {
        case NORMAL:      r=1.0f; g=0.1f; b=0.6f; break;
        case LASER:       r=0.0f; g=1.0f; b=1.0f; break;
        case BOOMERANG:   r=1.0f; g=0.6f; b=0.1f; break;
        case LONG_LASER:  r=1.0f; g=0.8f; b=0.0f; break;
        case TILE_DMG:    r=1.0f; g=0.2f; b=0.2f; break;
        case MOVING_LASER:r=0.2f; g=1.0f; b=0.2f; break;
        default:          r=1.0f; g=1.0f; b=1.0f; break;
    }
}

void SpawnAttack(float playerX, float playerY) {
    int   attackType = rand() % 16;
    float speed = 1.2f * globalSpeedMultiplier;
    float r = 1.0f, g = 0.1f, b = 0.6f;

    int   edge    = rand() % 4;
    float spawnX  = 0.0f, spawnY = 0.0f, dx = 0.0f, dy = 0.0f;
    float randPos = gridCoords[rand() % gridCells];

    if      (edge == 0) { spawnX = randPos; spawnY =  1.4f; dy = -speed; }
    else if (edge == 1) { spawnX = randPos; spawnY = -1.4f; dy =  speed; }
    else if (edge == 2) { spawnX = -1.4f; spawnY = randPos; dx =  speed; }
    else                { spawnX =  1.4f; spawnY = randPos; dx = -speed; }

    bool isHorizontal = (edge == 0 || edge == 1);

    if (attackType < 4) {
        attacks.push_back({ NORMAL,      spawnX, spawnY, dx,        dy,        r,    g,    b,    0,0,false,true, 0.3f,0.3f });
    } else if (attackType < 6) {
        if (isHorizontal) attacks.push_back({ LASER, 0.0f, gridCoords[rand()%gridCells], 0,0, 0,1,1, 0,0,false,false,2.8f,0.15f });
        else              attacks.push_back({ LASER, gridCoords[rand()%gridCells], 0.0f, 0,0, 0,1,1, 0,0,false,false,0.15f,2.8f });
    } else if (attackType < 8) {
        attacks.push_back({ BOOMERANG,   spawnX, spawnY, dx*1.2f,   dy*1.2f,   1.0f,0.6f,0.1f,0,0,false,true,0.3f,0.3f });
    } else if (attackType < 10) {
        if (isHorizontal) attacks.push_back({ LONG_LASER, 0.0f, gridCoords[rand()%gridCells], 0,0, 1,0.8f,0, 0,0,false,false,2.8f,0.15f });
        else              attacks.push_back({ LONG_LASER, gridCoords[rand()%gridCells], 0.0f, 0,0, 1,0.8f,0, 0,0,false,false,0.15f,2.8f });
    } else if (attackType < 12) {
        float rx = gridCoords[rand()%gridCells];
        float ry = gridCoords[rand()%gridCells];
        float sz = (rand()%2==0) ? 0.8f : 0.4f;
        attacks.push_back({ TILE_DMG,    rx, ry, 0,0, 1,0.2f,0.2f, 0,0,false,false,sz,sz });
    } else if (attackType < 14) {
        if (isHorizontal) attacks.push_back({ MOVING_LASER, 0.0f, spawnY, 0, dy*0.25f, 0.2f,1,0.2f, 0,0,false,true,2.8f,0.15f });
        else              attacks.push_back({ MOVING_LASER, spawnX, 0.0f, dx*0.25f, 0, 0.2f,1,0.2f, 0,0,false,true,0.15f,2.8f });
    } else {
        float tx = playerX - spawnX, ty = playerY - spawnY;
        float len = std::sqrt(tx*tx + ty*ty);
        if (len != 0) { tx /= len; ty /= len; }
        attacks.push_back({ NORMAL, spawnX, spawnY, tx*speed*1.5f, ty*speed*1.5f, 1,0,0.8f, 0,0,false,true,0.3f,0.3f });
    }
}

void SpawnFromEvent(const LevelEvent& ev, std::vector<Attack>& targetContainer) {
    float speed = 1.2f * globalSpeedMultiplier;
    float sx = 0, sy = 0, dx = 0, dy = 0;
    float tx = gridCoords[std::min(ev.gridCol, (int)gridCoords.size()-1)];
    float ty = gridCoords[std::min(ev.gridRow, (int)gridCoords.size()-1)];

    if      (ev.edge == 0) { sx = tx; sy =  1.4f; dy = -speed; }
    else if (ev.edge == 1) { sx = tx; sy = -1.4f; dy =  speed; }
    else if (ev.edge == 2) { sx = -1.4f; sy = ty; dx =  speed; }
    else if (ev.edge == 3) { sx =  1.4f; sy = ty; dx = -speed; }
    
    float w = 0.3f, h = 0.3f;
    
    if (ev.type == LASER || ev.type == LONG_LASER) {
        if (ev.edge == 0 || ev.edge == 1) { w = 2.8f; h = 0.15f; sx = 0.0f; sy = ty; dx = 0; dy = 0; }
        else                              { w = 0.15f; h = 2.8f; sx = tx; sy = 0.0f; dx = 0; dy = 0; }
    } 
    else if (ev.type == MOVING_LASER) {
        if (ev.edge == 0)      { sx = 0.0f; sy =  1.4f; dx = 0.0f;        dy = -speed * 0.25f; w = 2.8f; h = 0.15f; }
        else if (ev.edge == 1) { sx = 0.0f; sy = -1.4f; dx = 0.0f;        dy =  speed * 0.25f; w = 2.8f; h = 0.15f; }
        else if (ev.edge == 2) { sx = -1.4f; sy = 0.0f; dx =  speed * 0.25f; dy = 0.0f;        w = 0.15f; h = 2.8f; }
        else                   { sx =  1.4f; sy = 0.0f; dx = -speed * 0.25f; dy = 0.0f;        w = 0.15f; h = 2.8f; }
    } else if (ev.type == TILE_DMG) {
        w = 0.8f; h = 0.8f; sx = tx; sy = ty; dx = 0; dy = 0;
    }
    
    float r = 1.0f, g = 1.0f, b = 1.0f;
    GetAttackColor(ev.type, r, g, b);

    bool active = true;
    if (ev.type == LASER || ev.type == LONG_LASER || ev.type == TILE_DMG) active = false;

    targetContainer.push_back({ ev.type, sx, sy, dx, dy, r, g, b, 0.0f, 0.0f, false, active, w, h });
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
        int length; glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
        char* message = (char*)alloca(length * sizeof(char));
        glGetShaderInfoLog(id, length, &length, message);
        glDeleteShader(id); return 0;
    }
    return id;
}

static unsigned int CreateShader(const std::string& vs, const std::string& fs) {
    unsigned int program = glCreateProgram();
    unsigned int v = CompileShader(GL_VERTEX_SHADER, vs);
    unsigned int f = CompileShader(GL_FRAGMENT_SHADER, fs);
    glAttachShader(program, v); glAttachShader(program, f);
    glLinkProgram(program); glValidateProgram(program);
    glDeleteShader(v); glDeleteShader(f);
    return program;
}

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

void AddLine(std::vector<float>& v, float x1, float y1, float x2, float y2,
             float ox, float oy, float s) {
    v.push_back(ox+x1*s); v.push_back(oy+y1*s); v.push_back(0.0f);
    v.push_back(ox+x2*s); v.push_back(oy+y2*s); v.push_back(0.0f);
}

float TextWidth(const std::string& str, float scale) {
    if (str.empty()) return 0.0f;
    return ((float)str.size() * 1.6f - 0.6f) * scale;
}

float CenterX(const std::string& str, float scale, float centerX = 0.0f) {
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

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    
    float aspect = (height > 0) ? (float)width / (float)height : 1.0f;
    float x = (((mouseX / width)  * 2.0f - 1.0f) * aspect) / 0.7f;
    float y = (-((mouseY / height) * 2.0f - 1.0f)) / 0.7f;

    auto IsHovered = [x, y](float bx, float by, float bw, float bh) {
        return std::abs(x - bx) < bw / 2.0f && std::abs(y - by) < bh / 2.0f;
    };

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            if (currentState == MENU) {
                if (IsHovered(0.0f, 0.40f, 1.1f, 0.22f)) { ResetGame(window); currentState = GAME; levelMode = false; }
                else if (IsHovered(0.0f, 0.10f, 1.1f, 0.22f)) { currentState = SETTINGS; }
                else if (IsHovered(0.0f, -0.20f, 1.1f, 0.22f)) { currentState = LEVEL_EDITOR; }
                else if (IsHovered(0.0f, -0.50f, 1.1f, 0.22f)) { glfwSetWindowShouldClose(window, true); }
            } else if (currentState == SETTINGS || currentState == GAME_OVER) {
                if (IsHovered(0.0f, -0.78f, 1.1f, 0.25f)) { currentState = MENU; }
            } else if (currentState == LEVEL_EDITOR) {
                if (IsHovered(1.10f, -0.80f, 0.5f, 0.2f)) { currentState = MENU; }
                
                if (IsHovered(1.10f - 0.22f, -0.58f, 0.15f, 0.15f)) { 
                    currentLevel.duration = std::max(10.0f, currentLevel.duration - 10.0f); 
                    editorTime = std::min(editorTime, currentLevel.duration);
                }
                if (IsHovered(1.10f + 0.22f, -0.58f, 0.15f, 0.15f)) { 
                    currentLevel.duration += 10.0f; 
                }

                if (x >= -1.3f && x <= 0.3f && std::abs(y - (-0.85f)) < 0.1f) {
                    isDraggingTimeline = true;
                }
            }
        } 
        else if (action == GLFW_RELEASE) {
            isDraggingTimeline = false;
        }
    }
}

void ToggleFullscreen(GLFWwindow* window) {
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
        if (currentState == GAME) {
            if (key == GLFW_KEY_RIGHT && targetOffsetX < gridLimit - 0.01f) targetOffsetX += gridStep;
            if (key == GLFW_KEY_LEFT  && targetOffsetX > -gridLimit + 0.01f) targetOffsetX -= gridStep;
            if (key == GLFW_KEY_UP    && targetOffsetY < gridLimit - 0.01f) targetOffsetY += gridStep;
            if (key == GLFW_KEY_DOWN  && targetOffsetY > -gridLimit + 0.01f) targetOffsetY -= gridStep;
        }

        if (currentState == GAME_OVER && key == GLFW_KEY_R) {
            ResetGame(window); currentState = GAME;
        }

        if (currentState == LEVEL_EDITOR) {
            if (key == GLFW_KEY_RIGHT) editorCursorCol = std::min(editorCursorCol+1, gridCells-1);
            if (key == GLFW_KEY_LEFT)  editorCursorCol = std::max(editorCursorCol-1, 0);
            if (key == GLFW_KEY_DOWN)  editorCursorRow = std::max(editorCursorRow-1, 0);
            if (key == GLFW_KEY_UP)    editorCursorRow = std::min(editorCursorRow+1, gridCells-1);

            if (key == GLFW_KEY_COMMA)  editorTime = std::max(0.0f, editorTime - 0.5f);
            if (key == GLFW_KEY_PERIOD) editorTime = std::min(currentLevel.duration, editorTime + 0.5f);

            if (key == GLFW_KEY_R && action == GLFW_PRESS) {
                editorSelectedEdge = (editorSelectedEdge + 1) % 4;
            }

            if (key == GLFW_KEY_SPACE) {
                currentLevel.events.push_back({ editorSelectedType, editorTime, editorCursorCol, editorCursorRow, editorSelectedEdge });
                std::sort(currentLevel.events.begin(), currentLevel.events.end(),
                    [](const LevelEvent& a, const LevelEvent& b){ return a.triggerTime < b.triggerTime; });
                editorTime = std::min(currentLevel.duration, editorTime + 0.5f);
            }

            if (key == GLFW_KEY_BACKSPACE && !currentLevel.events.empty())
                currentLevel.events.pop_back();

            if (key == GLFW_KEY_1) editorSelectedType = NORMAL;
            if (key == GLFW_KEY_2) editorSelectedType = LASER;
            if (key == GLFW_KEY_3) editorSelectedType = BOOMERANG;
            if (key == GLFW_KEY_4) editorSelectedType = LONG_LASER;
            if (key == GLFW_KEY_5) editorSelectedType = TILE_DMG;
            if (key == GLFW_KEY_6) editorSelectedType = MOVING_LASER;

            if (key == GLFW_KEY_P && action == GLFW_PRESS) {
                editorPlaying = !editorPlaying;
                if (editorPlaying) {
                    editorPlayStart   = (float)glfwGetTime() - editorTime; 
                    editorNextEvent   = 0;
                    attacks.clear();
                    cubeOffsetX = targetOffsetX = 0;
                    cubeOffsetY = targetOffsetY = 0;
                }
            }

            if (key == GLFW_KEY_E && action == GLFW_PRESS) {
                std::cout << "\nLevel myLevel;\n";
                std::cout << "myLevel.duration = " << currentLevel.duration << "f;\n";
                for (auto& ev : currentLevel.events) {
                    std::cout << "myLevel.events.push_back({"
                        << ev.type << ", " << ev.triggerTime << "f, "
                        << ev.gridCol << ", " << ev.gridRow << ", " << ev.edge << "});\n";
                }
            }

            if (key == GLFW_KEY_DELETE && action == GLFW_PRESS) currentLevel.events.clear();
        }

        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) currentState = MENU;

        if (currentState == SETTINGS && key == GLFW_KEY_G && action == GLFW_PRESS) {
            int nextSize = gridCells + 2;
            if (nextSize > 15) nextSize = 5;
            SetGridSize(nextSize);
            UpdateGridVAO();
        }

        if (key == GLFW_KEY_F1 && action == GLFW_PRESS) { if (isFullscreen) ToggleFullscreen(window); glfwSetWindowSize(window, 600, 600); }
        if (key == GLFW_KEY_F2 && action == GLFW_PRESS) { if (isFullscreen) ToggleFullscreen(window); glfwSetWindowSize(window, 900, 900); }
        if (key == GLFW_KEY_F3 && action == GLFW_PRESS) { if (isFullscreen) ToggleFullscreen(window); glfwSetWindowSize(window, 1200, 900); }
        if (key == GLFW_KEY_F4 && action == GLFW_PRESS) { ToggleFullscreen(window); }
    }
}

const char* GetAttackName(AttackType t) {
    switch (t) {
        case NORMAL:      return "NORMAL";
        case LASER:       return "LASER";
        case BOOMERANG:   return "BOOMRNG";
        case LONG_LASER:  return "LONG-L";
        case TILE_DMG:    return "TILES";
        case MOVING_LASER:return "MOVE-L";
        default:          return "UNKNOWN";
    }
}

int main() {
    srand((unsigned)time(NULL));

    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(900, 900, "Grid Dodge", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    if (glewInit() != GLEW_OK) return -1;

    unsigned int shaderProgram = CreateShader(vertexShaderSource, fragmentShaderSource);

    float rectVertices[] = { -0.5f,-0.5f,0.0f,  0.5f,-0.5f,0.0f,  0.5f,0.5f,0.0f, -0.5f,0.5f,0.0f };
    unsigned int rectVAO, rectVBO;
    glGenVertexArrays(1, &rectVAO); glGenBuffers(1, &rectVBO);
    glBindVertexArray(rectVAO); glBindBuffer(GL_ARRAY_BUFFER, rectVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(rectVertices), rectVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    float quadVertices[] = { -0.5f,-0.5f,0.0f,  0.5f,-0.5f,0.0f,  -0.5f,0.5f,0.0f,  0.5f,0.5f,0.0f };
    unsigned int quadVAO, quadVBO;
    glGenVertexArrays(1, &quadVAO); glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO); glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glGenVertexArrays(1, &gridVAO); glGenBuffers(1, &gridVBO);
    UpdateGridVAO();
    glGenVertexArrays(1, &textVAO); glGenBuffers(1, &textVBO);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetKeyCallback(window, key_callback);

    int offsetLoc  = glGetUniformLocation(shaderProgram, "offset");
    int scaleLoc   = glGetUniformLocation(shaderProgram, "scale");
    int colorLoc   = glGetUniformLocation(shaderProgram, "color");
    int vOffLoc    = glGetUniformLocation(shaderProgram, "viewOffset");
    int vSclLoc    = glGetUniformLocation(shaderProgram, "viewScale");
    int aspectLoc  = glGetUniformLocation(shaderProgram, "aspect");

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);

    float lastFrame = 0.0f;

    auto DrawButton = [&](float x, float y, float w, float h, const std::string& text, float textScale, float r, float g, float b, bool hover) {
        glUniform2f(offsetLoc, x, y); glUniform2f(scaleLoc, w, h);
        glBindVertexArray(quadVAO);
        glUniform4f(colorLoc, r*0.15f, g*0.15f, b*0.15f, 0.8f);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        
        glBindVertexArray(rectVAO);
        glUniform4f(colorLoc, r, g, b, hover ? 1.0f : 0.5f);
        glLineWidth(hover ? 4.0f : 2.0f);
        glDrawArrays(GL_LINE_LOOP, 0, 4);
        
        glUniform2f(offsetLoc, 0.0f, 0.0f); glUniform2f(scaleLoc, 1.0f, 1.0f);
        glLineWidth(hover ? 3.0f : 2.0f);
        float textY = y - textScale * 0.9f;
        glUniform4f(colorLoc, hover ? r*1.2f : r, hover ? g*1.2f : g, hover ? b*1.2f : b, 1.0f);
        DrawDynamicLines(GenerateText(text, CenterX(text, textScale, x), textY, textScale));
    };

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = (float)glfwGetTime();
        float deltaTime    = currentFrame - lastFrame;
        lastFrame = currentFrame;

        int width, height;
        glfwGetWindowSize(window, &width, &height);
        float aspect = (height > 0) ? (float)width / (float)height : 1.0f;

        double mx, my; glfwGetCursorPos(window, &mx, &my);
        float hx = (((mx / width) * 2.0f - 1.0f) * aspect) / 0.7f;
        float hy = (-(((float)my / height) * 2.0f - 1.0f)) / 0.7f;
        auto Hover = [hx, hy](float bx, float by, float bw, float bh) {
            return std::abs(hx - bx) < bw/2.0f && std::abs(hy - by) < bh/2.0f;
        };

        if (currentState == LEVEL_EDITOR && isDraggingTimeline) {
            float ratio = (hx - (-1.3f)) / (0.3f - (-1.3f)); 
            editorTime = std::max(0.0f, std::min(ratio * currentLevel.duration, currentLevel.duration));
        }

        if (currentState == GAME) {
            gameTime += deltaTime;
            score = (int)(gameTime * 10.0f);

            cubeOffsetX += (targetOffsetX - cubeOffsetX) * 15.0f * deltaTime;
            cubeOffsetY += (targetOffsetY - cubeOffsetY) * 15.0f * deltaTime;

            globalSpeedMultiplier = 1.0f + gameTime * 0.05f;
            currentSpawnDelay = std::max(0.3f, 1.2f - gameTime * 0.02f);

            if (levelMode) {
                while (editorNextEvent < currentLevel.events.size() && currentLevel.events[editorNextEvent].triggerTime <= gameTime) {
                    SpawnFromEvent(currentLevel.events[editorNextEvent], attacks);
                    editorNextEvent++;
                }
            } else {
                if (currentFrame - lastAttackSpawn > currentSpawnDelay) {
                    SpawnAttack(targetOffsetX, targetOffsetY);
                    lastAttackSpawn = currentFrame;
                }
            }

            for (auto it = attacks.begin(); it != attacks.end(); ) {
                it->timer += deltaTime;
                float moveDist = std::sqrt(it->dx*it->dx + it->dy*it->dy) * deltaTime;
                it->distanceTravelled += moveDist;

                if (it->type == NORMAL) {
                    it->x += it->dx * deltaTime; it->y += it->dy * deltaTime;
                    if (checkCollision(it->x, it->y, it->width, it->height)) currentState = GAME_OVER;
                } else if (it->type == BOOMERANG) {
                    it->x += it->dx * deltaTime; it->y += it->dy * deltaTime;
                    if (!it->returning && it->distanceTravelled > 1.8f) {
                        it->dx *= -1.0f; it->dy *= -1.0f; it->returning = true;
                    }
                    if (checkCollision(it->x, it->y, it->width, it->height)) currentState = GAME_OVER;
                } else if (it->type == LASER) {
                    if (it->timer > 1.0f) { it->active = true; if (checkCollision(it->x, it->y, it->width, it->height)) currentState = GAME_OVER; }
                    if (it->timer > 1.5f) it->active = false;
                } else if (it->type == LONG_LASER) {
                    if (it->timer > 1.0f) { it->active = true; it->r=1; it->g=0.8f; it->b=0; if (checkCollision(it->x, it->y, it->width, it->height)) currentState = GAME_OVER; }
                    if (it->timer > 4.5f) it->active = false;
                } else if (it->type == TILE_DMG) {
                    if (it->timer > 1.2f) { it->active = true; it->r=1; it->g=0.2f; it->b=0.2f; if (checkCollision(it->x, it->y, it->width, it->height)) currentState = GAME_OVER; }
                    if (it->timer > 2.2f) it->active = false;
                } else if (it->type == MOVING_LASER) {
                    it->x += it->dx * deltaTime; it->y += it->dy * deltaTime;
                    if (checkCollision(it->x, it->y, it->width, it->height)) currentState = GAME_OVER;
                }

                bool remove = (it->y > 2.2f || it->y < -2.2f || it->x > 2.2f || it->x < -2.2f ||
                               (it->type == LASER       && it->timer > 1.6f) ||
                               (it->type == LONG_LASER  && it->timer > 4.6f) ||
                               (it->type == TILE_DMG    && it->timer > 2.3f) ||
                               (it->type == MOVING_LASER && it->distanceTravelled >= (0.6f + 2.0f * gridStep)));
                if (remove) it = attacks.erase(it); else ++it;
            }
        }

        if (currentState == LEVEL_EDITOR && editorPlaying) {
            float elapsed = currentFrame - editorPlayStart;
            editorTime = elapsed; 
            
            globalSpeedMultiplier = 1.0f;
            cubeOffsetX += (targetOffsetX - cubeOffsetX) * 15.0f * deltaTime;
            cubeOffsetY += (targetOffsetY - cubeOffsetY) * 15.0f * deltaTime;

            while (editorNextEvent < currentLevel.events.size() && currentLevel.events[editorNextEvent].triggerTime <= elapsed) {
                SpawnFromEvent(currentLevel.events[editorNextEvent], attacks);
                editorNextEvent++;
            }

            for (auto it = attacks.begin(); it != attacks.end(); ) {
                it->timer += deltaTime;
                it->x += it->dx * deltaTime; it->y += it->dy * deltaTime;
                if (it->y > 2.2f || it->y < -2.2f || it->x > 2.2f || it->x < -2.2f || it->timer > 5.0f)
                    it = attacks.erase(it);
                else ++it;
            }
            if (elapsed > currentLevel.duration) { 
                editorPlaying = false; 
                attacks.clear(); 
                editorTime = currentLevel.duration; 
            }
        }

        glClearColor(0.04f, 0.02f, 0.08f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(shaderProgram);
        glUniform1f(aspectLoc, aspect);
        glUniform2f(vOffLoc, 0.0f, 0.0f);
        glUniform2f(vSclLoc, 1.0f, 1.0f);

        if (currentState == MENU) {
            float t = currentFrame;
            float pulse = 0.5f + 0.5f * sinf(t * 2.0f);
            glUniform2f(offsetLoc, 0.0f, 0.0f); glUniform2f(scaleLoc, 1.0f, 1.0f);
            glLineWidth(8.0f);
            glUniform4f(colorLoc, 1.0f, pulse * 0.1f, 0.7f + pulse * 0.3f, 0.3f);
            DrawDynamicLines(GenerateText("GRID DODGE", CenterX("GRID DODGE", 0.15f), 0.65f, 0.15f));
            glLineWidth(4.0f);
            glUniform4f(colorLoc, 1.0f, 0.4f, 1.0f, 1.0f);
            DrawDynamicLines(GenerateText("GRID DODGE", CenterX("GRID DODGE", 0.15f), 0.65f, 0.15f));

            DrawButton(0.0f, 0.40f, 1.1f, 0.22f, "PLAY", 0.08f, 0.0f, 1.0f, 1.0f, Hover(0.0f, 0.40f, 1.1f, 0.22f));
            DrawButton(0.0f, 0.10f, 1.1f, 0.22f, "SETTINGS", 0.08f, 0.5f, 0.3f, 1.0f, Hover(0.0f, 0.10f, 1.1f, 0.22f));
            DrawButton(0.0f, -0.20f, 1.1f, 0.22f, "EDITOR", 0.08f, 0.0f, 0.8f, 0.5f, Hover(0.0f, -0.20f, 1.1f, 0.22f));
            DrawButton(0.0f, -0.50f, 1.1f, 0.22f, "QUIT", 0.08f, 1.0f, 0.1f, 0.2f, Hover(0.0f, -0.50f, 1.1f, 0.22f));
        }

        else if (currentState == SETTINGS) {
            glUniform2f(offsetLoc,0.0f,0.0f); glUniform2f(scaleLoc,1.0f,1.0f);
            glLineWidth(5.0f);
            glUniform4f(colorLoc, 0.5f, 0.3f, 1.0f, 1.0f);
            DrawDynamicLines(GenerateText("SETTINGS", CenterX("SETTINGS", 0.12f), 0.65f, 0.12f));

            glLineWidth(2.5f);
            glUniform4f(colorLoc, 0.7f, 0.8f, 1.0f, 1.0f);
            float scale = 0.06f;
            DrawDynamicLines(GenerateText("G    CHANGE GRID SIZE", CenterX("G    CHANGE GRID SIZE", scale), 0.35f, scale));
            DrawDynamicLines(GenerateText("F1   WINDOW 600X600",    CenterX("F1   WINDOW 600X600", scale), 0.15f, scale));
            DrawDynamicLines(GenerateText("F2   WINDOW 900X900",    CenterX("F2   WINDOW 900X900", scale),-0.05f, scale));
            DrawDynamicLines(GenerateText("F3   WINDOW 1200X900",   CenterX("F3   WINDOW 1200X900", scale),-0.25f, scale));
            DrawDynamicLines(GenerateText("F4   FULLSCREEN",        CenterX("F4   FULLSCREEN", scale),-0.45f, scale));

            glLineWidth(2.0f);
            glUniform4f(colorLoc, 0.4f, 0.9f, 0.8f, 1.0f);
            std::string gs = "GRID: "; gs += std::to_string(gridCells); gs += "X"; gs += std::to_string(gridCells);
            DrawDynamicLines(GenerateText(gs, CenterX(gs, 0.07f), -0.58f, 0.07f));

            DrawButton(0.0f, -0.78f, 1.1f, 0.25f, "MENU", 0.07f, 1.0f, 0.0f, 0.8f, Hover(0.0f,-0.78f, 1.1f, 0.25f));
        }

        else if (currentState == GAME) {
            glBindVertexArray(gridVAO);
            glUniform2f(offsetLoc,0.0f,0.0f); glUniform2f(scaleLoc,1.0f,1.0f);
            glUniform4f(colorLoc, 0.2f,0.1f,0.6f,0.6f);
            glDrawArrays(GL_LINES, 0, (GLsizei)(gridCoords.size()*4));

            float pulse = 0.8f + 0.2f * sinf(currentFrame * 6.0f);
            glBindVertexArray(quadVAO);
            glUniform2f(offsetLoc, cubeOffsetX, cubeOffsetY);
            glUniform2f(scaleLoc,  0.3f, 0.3f);
            glUniform4f(colorLoc, 0.0f, pulse, pulse, 1.0f);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            for (const auto& atk : attacks) {
                glUniform2f(offsetLoc, atk.x, atk.y);
                if (atk.type == LASER || atk.type == LONG_LASER) {
                    if (atk.width > atk.height) glUniform2f(scaleLoc, atk.width, atk.active?atk.height:0.03f);
                    else                         glUniform2f(scaleLoc, atk.active?atk.width:0.03f, atk.height);
                } else if (atk.type == TILE_DMG) {
                    glUniform2f(scaleLoc, atk.active?atk.width:atk.width*0.7f, atk.active?atk.height:atk.height*0.7f);
                } else {
                    glUniform2f(scaleLoc, atk.width, atk.height);
                }
                glUniform4f(colorLoc, atk.r, atk.g, atk.b, atk.active?1.0f:0.4f);
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            }

            glUniform2f(offsetLoc,0.0f,0.0f); glUniform2f(scaleLoc,1.0f,1.0f);
            glLineWidth(2.5f);
            glUniform4f(colorLoc, 0.2f,0.9f,1.0f,0.9f);
            std::string sc = "SCORE: "; sc += std::to_string(score);
            DrawDynamicLines(GenerateText(sc, CenterX(sc, 0.07f), 0.88f, 0.07f));
        }

        else if (currentState == GAME_OVER) {
            glUniform2f(offsetLoc,0.0f,0.0f); glUniform2f(scaleLoc,1.0f,1.0f);

            float blink = 0.6f + 0.4f * sinf(currentFrame * 3.0f);
            glLineWidth(8.0f);
            glUniform4f(colorLoc, 1.0f, 0.1f, 0.3f, blink * 0.4f);
            DrawDynamicLines(GenerateText("GAME OVER", CenterX("GAME OVER", 0.13f), 0.45f, 0.13f));
            glLineWidth(4.0f);
            glUniform4f(colorLoc, 1.0f, 0.2f, 0.4f, 1.0f);
            DrawDynamicLines(GenerateText("GAME OVER", CenterX("GAME OVER", 0.13f), 0.45f, 0.13f));

            glLineWidth(3.0f);
            glUniform4f(colorLoc, 0.0f,1.0f,1.0f,1.0f);
            std::string sc = "SCORE: "; sc += std::to_string(score);
            DrawDynamicLines(GenerateText(sc, CenterX(sc, 0.09f), 0.10f, 0.09f));

            glLineWidth(2.0f);
            glUniform4f(colorLoc, 0.6f,0.4f,1.0f,0.8f);
            DrawDynamicLines(GenerateText("R    RESTART", CenterX("R    RESTART", 0.06f), -0.15f, 0.06f));

            DrawButton(0.0f, -0.78f, 1.1f, 0.25f, "MENU", 0.07f, 1.0f, 0.0f, 0.8f, Hover(0.0f,-0.78f, 1.1f, 0.25f));
        }

        else if (currentState == LEVEL_EDITOR) {
            glUniform2f(vOffLoc, -0.45f, 0.1f); 
            glUniform2f(vSclLoc, 0.55f, 0.55f);

            std::vector<float> outerGridLines;
            AddLine(outerGridLines, -1.4f, -1.4f,  1.4f, -1.4f, 0.0f, 0.0f, 1.0f);
            AddLine(outerGridLines,  1.4f, -1.4f,  1.4f,  1.4f, 0.0f, 0.0f, 1.0f);
            AddLine(outerGridLines,  1.4f,  1.4f, -1.4f,  1.4f, 0.0f, 0.0f, 1.0f);
            AddLine(outerGridLines, -1.4f,  1.4f, -1.4f, -1.4f, 0.0f, 0.0f, 1.0f);
            for (float x : gridCoords) {
                AddLine(outerGridLines, x,  0.8f, x,  1.4f, 0.0f, 0.0f, 1.0f);
                AddLine(outerGridLines, x, -0.8f, x, -1.4f, 0.0f, 0.0f, 1.0f);
            }
            for (float y : gridCoords) {
                AddLine(outerGridLines, -0.8f, y, -1.4f, y, 0.0f, 0.0f, 1.0f);
                AddLine(outerGridLines,  0.8f, y,  1.4f, y, 0.0f, 0.0f, 1.0f);
            }
            glLineWidth(1.5f);
            glUniform4f(colorLoc, 0.4f, 0.4f, 0.4f, 0.3f); 
            DrawDynamicLines(outerGridLines);

            glBindVertexArray(gridVAO);
            glUniform2f(offsetLoc,0.0f,0.0f); glUniform2f(scaleLoc,1.0f,1.0f);
            glUniform4f(colorLoc, 0.15f,0.1f,0.5f,0.7f);
            glDrawArrays(GL_LINES, 0, (GLsizei)(gridCoords.size()*4));

            float cx = gridCoords[editorCursorCol];
            float cy = gridCoords[editorCursorRow];
            float cursorPulse = 0.5f + 0.5f*sinf(currentFrame*4.0f);
            
            glBindVertexArray(rectVAO);
            glUniform2f(offsetLoc, cx, cy); glUniform2f(scaleLoc,0.35f,0.35f);
            glUniform4f(colorLoc, 0.0f, 1.0f, cursorPulse, 0.9f);
            glDrawArrays(GL_LINE_LOOP,0,4);

            std::vector<Attack> previewAttacks;

            if (!editorPlaying) {
                LevelEvent tempEvent = { editorSelectedType, editorTime, editorCursorCol, editorCursorRow, editorSelectedEdge };
                SpawnFromEvent(tempEvent, previewAttacks);
            }

            for (const auto& ev : currentLevel.events) {
                float lifeTime = 0.0f;
                if (ev.type == LASER) lifeTime = 1.6f;
                else if (ev.type == LONG_LASER) lifeTime = 4.6f;
                else if (ev.type == TILE_DMG) lifeTime = 2.3f;
                else lifeTime = 3.0f; 

                if (editorTime >= ev.triggerTime && editorTime <= (ev.triggerTime + lifeTime)) {
                    std::vector<Attack> tmp;
                    SpawnFromEvent(ev, tmp);
                    if (!tmp.empty()) {
                        tmp[0].timer = (editorTime - ev.triggerTime);
                        
                        if (tmp[0].type == NORMAL || tmp[0].type == BOOMERANG || tmp[0].type == MOVING_LASER) {
                            float t = tmp[0].timer;
                            if (tmp[0].type == BOOMERANG) {
                                float dist = std::sqrt(tmp[0].dx*tmp[0].dx + tmp[0].dy*tmp[0].dy) * t;
                                if (dist > 1.8f) {
                                    float fullSpeedTime = 1.8f / std::sqrt(tmp[0].dx*tmp[0].dx + tmp[0].dy*tmp[0].dy);
                                    float returnTime = t - fullSpeedTime;
                                    tmp[0].x += tmp[0].dx * fullSpeedTime - tmp[0].dx * returnTime;
                                    tmp[0].y += tmp[0].dy * fullSpeedTime - tmp[0].dy * returnTime;
                                } else {
                                    tmp[0].x += tmp[0].dx * t;
                                    tmp[0].y += tmp[0].dy * t;
                                }
                            } else {
                                tmp[0].x += tmp[0].dx * t;
                                tmp[0].y += tmp[0].dy * t;
                            }
                        }
                        
                        if (tmp[0].type == LASER && tmp[0].timer > 1.0f && tmp[0].timer <= 1.5f) tmp[0].active = true;
                        if (tmp[0].type == LASER && tmp[0].timer > 1.5f) tmp[0].active = false;
                        if (tmp[0].type == LONG_LASER && tmp[0].timer > 1.0f && tmp[0].timer <= 4.5f) tmp[0].active = true;
                        if (tmp[0].type == LONG_LASER && tmp[0].timer > 4.5f) tmp[0].active = false;
                        if (tmp[0].type == TILE_DMG && tmp[0].timer > 1.2f && tmp[0].timer <= 2.2f) tmp[0].active = true;
                        if (tmp[0].type == TILE_DMG && tmp[0].timer > 2.2f) tmp[0].active = false;

                        previewAttacks.push_back(tmp[0]);
                    }
                }
            }

            glBindVertexArray(quadVAO);
            for (const auto& atk : previewAttacks) {
                glUniform2f(offsetLoc, atk.x, atk.y);
                if (atk.type == LASER || atk.type == LONG_LASER) {
                    if (atk.width > atk.height) glUniform2f(scaleLoc, atk.width, atk.active ? atk.height : 0.03f);
                    else                         glUniform2f(scaleLoc, atk.active ? atk.width : 0.03f, atk.height);
                } else if (atk.type == TILE_DMG) {
                    glUniform2f(scaleLoc, atk.active ? atk.width : atk.width * 0.7f, atk.active ? atk.height : atk.height * 0.7f);
                } else {
                    glUniform2f(scaleLoc, atk.width, atk.height);
                }
                glUniform4f(colorLoc, atk.r, atk.g, atk.b, atk.active ? 0.7f : 0.3f);
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            }

            glUniform2f(vOffLoc, 0.0f, 0.0f);
            glUniform2f(vSclLoc, 1.0f, 1.0f);
            glUniform2f(offsetLoc,0.0f,0.0f); glUniform2f(scaleLoc,1.0f,1.0f);

            float uiX = 1.10f;
            glLineWidth(4.0f); glUniform4f(colorLoc,0.0f,0.9f,0.5f,1.0f);
            DrawDynamicLines(GenerateText("EDITOR", CenterX("EDITOR", 0.09f, uiX), 0.80f, 0.09f));

            glUniform4f(colorLoc,0.8f,0.8f,0.2f,1.0f);
            std::string typeStr = "TYPE: " + std::string(GetAttackName(editorSelectedType));
            DrawDynamicLines(GenerateText(typeStr, CenterX(typeStr, 0.055f, uiX), 0.60f, 0.055f));

            glUniform4f(colorLoc, editorPlaying?0.2f:0.5f, editorPlaying?1.0f:0.5f, editorPlaying?0.2f:0.5f, 1.0f);
            const char* statusStr = editorPlaying ? "PLAYING" : "PAUSED";
            DrawDynamicLines(GenerateText(statusStr, CenterX(statusStr, 0.055f, uiX), 0.45f, 0.055f));

            glUniform4f(colorLoc, 1.0f, 1.0f, 0.0f, 1.0f);
            std::string edgeStr = "FROM: ";
            if      (editorSelectedEdge == 0) edgeStr += "TOP";
            else if (editorSelectedEdge == 1) edgeStr += "BOTTOM";
            else if (editorSelectedEdge == 2) edgeStr += "LEFT";
            else                              edgeStr += "RIGHT";
            DrawDynamicLines(GenerateText(edgeStr, CenterX(edgeStr, 0.055f, uiX), 0.32f, 0.055f));

            std::vector<float> uiArrow;
            float ay = 0.20f; float arrowSize = 0.06f;
            if (editorSelectedEdge == 0) { 
                AddLine(uiArrow, 0.0f, arrowSize, 0.0f, -arrowSize, uiX, ay, 1.0f);
                AddLine(uiArrow, 0.0f, -arrowSize, -arrowSize * 0.5f, -arrowSize * 0.2f, uiX, ay, 1.0f);
                AddLine(uiArrow, 0.0f, -arrowSize,  arrowSize * 0.5f, -arrowSize * 0.2f, uiX, ay, 1.0f);
            } else if (editorSelectedEdge == 1) { 
                AddLine(uiArrow, 0.0f, -arrowSize, 0.0f, arrowSize, uiX, ay, 1.0f);
                AddLine(uiArrow, 0.0f, arrowSize, -arrowSize * 0.5f, arrowSize * 0.2f, uiX, ay, 1.0f);
                AddLine(uiArrow, 0.0f, arrowSize,  arrowSize * 0.5f, arrowSize * 0.2f, uiX, ay, 1.0f);
            } else if (editorSelectedEdge == 2) { 
                AddLine(uiArrow, -arrowSize, 0.0f, arrowSize, 0.0f, uiX, ay, 1.0f);
                AddLine(uiArrow, arrowSize, 0.0f, arrowSize * 0.2f, -arrowSize * 0.5f, uiX, ay, 1.0f);
                AddLine(uiArrow, arrowSize, 0.0f, arrowSize * 0.2f,  arrowSize * 0.5f, uiX, ay, 1.0f);
            } else { 
                AddLine(uiArrow, arrowSize, 0.0f, -arrowSize, 0.0f, uiX, ay, 1.0f);
                AddLine(uiArrow, -arrowSize, 0.0f, -arrowSize * 0.2f, -arrowSize * 0.5f, uiX, ay, 1.0f);
                AddLine(uiArrow, -arrowSize, 0.0f, -arrowSize * 0.2f,  arrowSize * 0.5f, uiX, ay, 1.0f);
            }
            glLineWidth(4.0f); DrawDynamicLines(uiArrow);

            glLineWidth(1.5f); glUniform4f(colorLoc,0.3f,0.5f,0.3f,0.7f);
            DrawDynamicLines(GenerateText("SPACE  ADD",   CenterX("SPACE  ADD", 0.045f, uiX),  0.04f, 0.045f));
            DrawDynamicLines(GenerateText("1-6    TYPE",  CenterX("1-6    TYPE",0.045f, uiX), -0.04f, 0.045f));
            DrawDynamicLines(GenerateText("R      ROTATE",CenterX("R      ROTATE",0.045f, uiX), -0.12f, 0.045f));
            DrawDynamicLines(GenerateText("P      PREVIEW",CenterX("P      PREVIEW",0.045f, uiX), -0.20f, 0.045f));
            DrawDynamicLines(GenerateText("DEL    CLEAR",  CenterX("DEL    CLEAR",0.045f, uiX),  -0.28f, 0.045f));
            DrawDynamicLines(GenerateText("E      EXPORT", CenterX("E      EXPORT",0.045f, uiX), -0.36f, 0.045f));

            std::string durStr = "MAX: " + std::to_string((int)currentLevel.duration) + "s";
            DrawDynamicLines(GenerateText(durStr, CenterX(durStr, 0.05f, uiX), -0.46f, 0.05f));
            DrawButton(uiX - 0.22f, -0.58f, 0.15f, 0.15f, "-", 0.07f, 0.8f, 0.2f, 0.2f, Hover(uiX - 0.22f, -0.58f, 0.15f, 0.15f));
            DrawButton(uiX + 0.22f, -0.58f, 0.15f, 0.15f, "+", 0.07f, 0.2f, 0.8f, 0.2f, Hover(uiX + 0.22f, -0.58f, 0.15f, 0.15f));
            DrawButton(uiX, -0.80f, 0.5f, 0.2f, "MENU", 0.07f, 0.0f, 0.8f, 0.5f, Hover(uiX, -0.80f, 0.5f, 0.2f));

            float TL_START = -1.3f;
            float TL_END = 0.3f;
            float TL_Y = -0.85f;

            glLineWidth(2.0f); glUniform4f(colorLoc,1.0f,1.0f,1.0f,1.0f);
            char tBuf[32]; snprintf(tBuf, sizeof(tBuf), "TIME: %.1f s", editorTime);
            DrawDynamicLines(GenerateText(tBuf, TL_START, TL_Y + 0.15f, 0.06f));

            glLineWidth(3.0f); glUniform4f(colorLoc,0.4f,0.4f,0.4f,1.0f);
            std::vector<float> tlLine;
            AddLine(tlLine, 0, 0, 1, 0, TL_START, TL_Y, TL_END - TL_START);
            DrawDynamicLines(tlLine);

            glLineWidth(2.0f);
            for (const auto& ev : currentLevel.events) {
                float tx = TL_START + (ev.triggerTime / currentLevel.duration) * (TL_END - TL_START);
                float er,eg,eb; GetAttackColor(ev.type,er,eg,eb);
                glUniform4f(colorLoc, er, eg, eb, 0.8f);
                std::vector<float> singleTick;
                AddLine(singleTick, 0, -0.5f, 0, 0.5f, tx, TL_Y, 0.08f);
                DrawDynamicLines(singleTick);
            }

            float sliderX = TL_START + (editorTime / currentLevel.duration) * (TL_END - TL_START);
            glLineWidth(4.0f); glUniform4f(colorLoc,0.0f,1.0f,1.0f,1.0f);
            std::vector<float> sliderLine;
            AddLine(sliderLine, 0, -1.0f, 0, 1.0f, sliderX, TL_Y, 0.12f);
            DrawDynamicLines(sliderLine);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1,&quadVAO); glDeleteBuffers(1,&quadVBO);
    glDeleteVertexArrays(1,&rectVAO); glDeleteBuffers(1,&rectVBO);
    glDeleteVertexArrays(1,&gridVAO); glDeleteBuffers(1,&gridVBO);
    glDeleteVertexArrays(1,&textVAO); glDeleteBuffers(1,&textVBO);
    glDeleteProgram(shaderProgram);
    glfwTerminate();
    return 0;
}