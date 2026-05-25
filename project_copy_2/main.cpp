/**
 * @file main.cpp
 * @brief Entry point and rendering loop for Grid Dodge.
 */

#include "common.h"

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