/**
 * @file common.h
 * @brief Shared declarations, structures and constants used across the project.
 */

#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <algorithm>

// Enums
enum GameState { MENU, SETTINGS, GAME, GAME_OVER, LEVEL_EDITOR };
enum AttackType { NORMAL, LASER, BOOMERANG, LONG_LASER, TILE_DMG, MOVING_LASER };

// Structs
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

// Externí globální proměnné
extern GameState currentState;
extern AttackType editorSelectedType;
extern float cubeOffsetX;
extern float cubeOffsetY;
extern float targetOffsetX;
extern float targetOffsetY;

extern int gridCells;
extern std::vector<float> gridCoords;
extern float gridLimit;
extern float gridStep;

extern std::vector<Attack> attacks;

extern float gameTime;
extern float lastAttackSpawn;
extern float currentSpawnDelay;
extern float globalSpeedMultiplier;
extern int   score;

extern Level        currentLevel;
extern int          editorCursorCol;
extern int          editorCursorRow;
extern float        editorTime;
extern int          editorSelectedEdge; 
extern bool         editorPlaying;
extern float         editorPlayStart;
extern size_t       editorNextEvent;
extern bool         levelMode; 
extern bool         isDraggingTimeline; 

extern bool isFullscreen;
extern int  savedWidth, savedHeight;
extern int  savedX, savedY;

extern unsigned int gridVAO, gridVBO;
extern unsigned int textVAO, textVBO;

extern std::string vertexShaderSource;
extern std::string fragmentShaderSource;

void SetGridSize(int size);
void UpdateGridVAO();
void ResetGame(GLFWwindow* window);
void GetAttackColor(AttackType t, float& r, float& g, float& b);
void SpawnAttack(float playerX, float playerY);
void SpawnFromEvent(const LevelEvent& ev, std::vector<Attack>& targetContainer);
bool checkCollision(float ax, float ay, float aw, float ah);
const char* GetAttackName(AttackType t);

unsigned int CreateShader(const std::string& vs, const std::string& fs);
void AddLine(std::vector<float>& v, float x1, float y1, float x2, float y2, float ox, float oy, float s);
float TextWidth(const std::string& str, float scale);
float CenterX(const std::string& str, float scale, float centerX = 0.0f);
std::vector<float> GenerateText(const std::string& str, float startX, float startY, float scale);
void DrawDynamicLines(const std::vector<float>& pts);

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void ToggleFullscreen(GLFWwindow* window);