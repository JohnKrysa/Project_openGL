/**
 * @file globals.cpp
 * @brief Definitions of globally shared variables.
 */

#include "common.h"

GameState currentState = MENU;
AttackType editorSelectedType = NORMAL;

float cubeOffsetX = 0.0f;
float cubeOffsetY = 0.0f;
float targetOffsetX = 0.0f;
float targetOffsetY = 0.0f;

int gridCells = 5;
std::vector<float> gridCoords = { -0.8f, -0.4f, 0.0f, 0.4f, 0.8f };
float gridLimit = 0.8f;
float gridStep  = 0.4f;

std::vector<Attack> attacks;

float gameTime            = 0.0f;
float lastAttackSpawn     = 0.0f;
float currentSpawnDelay   = 1.2f;
float globalSpeedMultiplier = 1.0f;
int   score               = 0;

Level        currentLevel;
int          editorCursorCol      = 0;
int          editorCursorRow      = 0;
float        editorTime           = 0.0f;
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