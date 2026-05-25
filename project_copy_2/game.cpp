#include "common.h"

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