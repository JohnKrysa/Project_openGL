/**
 * @file main.cpp
 * @brief Vstupní bod aplikace, hlavní herní smyčka a veškeré vykreslování.
 *
 * Inicializuje GLFW okno, OpenGL kontext (3.3 Core Profile) a GLEW.
 * Spravuje hlavní smyčku (game loop) s pevnou logikou a variabilním renderem.
 *
 * Struktura main():
 * 1. Inicializace GLFW, okna, GLEW, audio enginu
 * 2. Vytvoření shader programu a VAO/VBO (rect, quad, grid, text)
 * 3. Registrace GLFW callbacků
 * 4. Hlavní smyčka:
 *    - Aktualizace audio (fade-out)
 *    - Herní logika podle currentState (pohyb hráče, spawn útoků, kolize)
 *    - Renderování (mřížka, hráč, útoky, UI)
 * 5. Úklid (AudioShutdown, glfwTerminate)
 */
#include "common.h"
#include "audio.h"

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

    
    AudioInit();
    
    menuMusicPath = "audio/menu.mp3";
    
    AudioPlayLoop(menuMusicPath, (float)audioVolumeLevel / 10.0f);

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
    glfwSetCharCallback(window, char_callback);

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
        float textY = y - textScale * 1.05f;
        glUniform4f(colorLoc, hover ? std::min(r*1.3f,1.0f) : r, hover ? std::min(g*1.3f,1.0f) : g, hover ? std::min(b*1.3f,1.0f) : b, 1.0f);
        DrawDynamicLines(GenerateText(text, CenterX(text, textScale, x), textY, textScale));
    };

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = (float)glfwGetTime();
        float deltaTime    = currentFrame - lastFrame;
        lastFrame = currentFrame;

        
        AudioUpdate(deltaTime);

        int width, height;
        glfwGetWindowSize(window, &width, &height);
        float aspect = (height > 0) ? (float)width / (float)height : 1.0f;

        double mx, my; glfwGetCursorPos(window, &mx, &my);
        // Převod pozice kurzoru z pixelů do souřadnicového prostoru scény.
        // Koeficient 0.7 odpovídá stejnému "zoom" faktoru jako ve vertex shaderu
        // (gl_Position = vec4(pos.x / aspect * 0.7, pos.y * 0.7, ...)).
        const float kViewZoom = 0.7f;
        float hx = (((mx / width) * 2.0f - 1.0f) * aspect) / kViewZoom;
        float hy = (-(((float)my / height) * 2.0f - 1.0f)) / kViewZoom;
        auto Hover = [hx, hy](float bx, float by, float bw, float bh) {
            return std::abs(hx - bx) < bw/2.0f && std::abs(hy - by) < bh/2.0f;
        };

        if (currentState == LEVEL_EDITOR && isDraggingTimeline) {
            // Časová osa editoru sahá od TL_START do TL_END ve souřadnicích scény
            const float TL_START = -1.25f;
            const float TL_END   =  1.25f;
            float ratio = (hx - TL_START) / (TL_END - TL_START); 
            editorTime = std::max(0.0f, std::min(ratio * currentLevel.duration, currentLevel.duration));
        }

        if (currentState == GAME) {
            gameTime += deltaTime;
            if (!levelMode) score = (int)(gameTime * 10.0f);

            cubeOffsetX += (targetOffsetX - cubeOffsetX) * 15.0f * deltaTime;
            cubeOffsetY += (targetOffsetY - cubeOffsetY) * 15.0f * deltaTime;

            globalSpeedMultiplier = 1.0f + gameTime * 0.05f;
            currentSpawnDelay = std::max(0.3f, 1.2f - gameTime * 0.02f);

            if (levelMode) {
                if (gameTime >= currentLevel.duration) {
                    attacks.clear();
                    currentState = LEVEL_COMPLETE;
                    
                    AudioStop(600);
                    AudioPlayLoop(menuMusicPath, (float)audioVolumeLevel / 10.0f);
                } else {
                    while (editorNextEvent < currentLevel.events.size() && currentLevel.events[editorNextEvent].triggerTime <= gameTime) {
                        SpawnFromEvent(currentLevel.events[editorNextEvent], attacks);
                        editorNextEvent++;
                    }
                }
            } else {
                if (currentFrame - lastAttackSpawn > currentSpawnDelay) {
                    SpawnAttack(targetOffsetX, targetOffsetY);
                    lastAttackSpawn = currentFrame;
                }
            }

            for (auto it = attacks.begin(); it != attacks.end(); ) {
                UpdateAttack(*it, deltaTime);
                if (ShouldRemoveAttack(*it)) it = attacks.erase(it); else ++it;
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
            glLineWidth(7.0f);
            glUniform4f(colorLoc, 1.0f, pulse * 0.1f, 0.7f + pulse * 0.3f, 0.3f);
            DrawDynamicLines(GenerateText("GRID DODGE", CenterX("GRID DODGE", 0.11f), 0.80f, 0.11f));
            glLineWidth(3.5f);
            glUniform4f(colorLoc, 1.0f, 0.4f, 1.0f, 1.0f);
            DrawDynamicLines(GenerateText("GRID DODGE", CenterX("GRID DODGE", 0.11f), 0.80f, 0.11f));

            const float BH = 0.22f, BW = 1.35f, BSTEP = 0.28f;
            float by = 0.46f;
            DrawButton(0.0f, by, BW, BH, "PLAY",         0.072f, 0.0f, 1.0f, 1.0f, Hover(0.0f, by, BW, BH)); by -= BSTEP;
            DrawButton(0.0f, by, BW, BH, "CUSTOM LEVELS", 0.060f, 1.0f, 0.8f, 0.2f, Hover(0.0f, by, BW, BH)); by -= BSTEP;
            DrawButton(0.0f, by, BW, BH, "SETTINGS",     0.072f, 0.5f, 0.3f, 1.0f, Hover(0.0f, by, BW, BH)); by -= BSTEP;
            DrawButton(0.0f, by, BW, BH, "EDITOR",       0.072f, 0.0f, 0.8f, 0.5f, Hover(0.0f, by, BW, BH)); by -= BSTEP;
            DrawButton(0.0f, by, BW, BH, "QUIT",         0.072f, 1.0f, 0.1f, 0.2f, Hover(0.0f, by, BW, BH));
        }

        else if (currentState == CUSTOM_LEVELS) {
            glUniform2f(offsetLoc, 0.0f, 0.0f); glUniform2f(scaleLoc, 1.0f, 1.0f);

            glLineWidth(5.0f);
            glUniform4f(colorLoc, 1.0f, 0.8f, 0.2f, 1.0f);
            DrawDynamicLines(GenerateText("CUSTOM LEVELS", CenterX("CUSTOM LEVELS", 0.080f), 0.82f, 0.080f));

            glLineWidth(2.0f); glUniform4f(colorLoc, 1.0f, 0.8f, 0.2f, 0.4f);
            { std::vector<float> sep; AddLine(sep, 0,0,1,0, -0.75f, 0.70f, 1.50f); DrawDynamicLines(sep); }

            const float LIST_W  = 1.30f;
            const float LBH     = 0.185f;
            const float LBSTEP  = 0.215f;
            const float LIST_TOP = 0.55f;
            const int   VISIBLE  = 5;
            
            int total = (int)customLevelFiles.size();

            if (total == 0) {
                glUniform2f(offsetLoc, 0.0f, 0.08f); glUniform2f(scaleLoc, 1.44f, 0.70f);
                glBindVertexArray(quadVAO);
                glUniform4f(colorLoc, 0.06f, 0.04f, 0.02f, 0.80f);
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                glBindVertexArray(rectVAO);
                glUniform4f(colorLoc, 0.5f, 0.4f, 0.1f, 0.50f);
                glLineWidth(1.5f); glDrawArrays(GL_LINE_LOOP, 0, 4);
                glUniform2f(offsetLoc, 0.0f, 0.0f); glUniform2f(scaleLoc, 1.0f, 1.0f);
                glLineWidth(2.5f); glUniform4f(colorLoc, 0.8f, 0.8f, 0.6f, 1.0f);
                DrawDynamicLines(GenerateText("NO LEVELS FOUND", CenterX("NO LEVELS FOUND", 0.052f), 0.24f, 0.052f));
                glLineWidth(1.8f); glUniform4f(colorLoc, 0.5f, 0.5f, 0.4f, 0.8f);
                DrawDynamicLines(GenerateText("USE IMPORT OR PLACE", CenterX("USE IMPORT OR PLACE", 0.038f), 0.08f, 0.038f));
                DrawDynamicLines(GenerateText(".TXT FILES IN levels/", CenterX(".TXT FILES IN levels/", 0.038f), -0.10f, 0.038f));
            } else {
                float boxH = VISIBLE * LBSTEP + 0.04f;
                float boxCY = LIST_TOP - boxH * 0.5f + LBH * 0.5f;
                glUniform2f(offsetLoc, 0.0f, boxCY); glUniform2f(scaleLoc, LIST_W + 0.06f, boxH);
                glBindVertexArray(quadVAO);
                glUniform4f(colorLoc, 0.03f, 0.04f, 0.06f, 0.7f);
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                glBindVertexArray(rectVAO);
                glUniform4f(colorLoc, 0.3f, 0.5f, 0.8f, 0.35f);
                glLineWidth(1.5f); glDrawArrays(GL_LINE_LOOP, 0, 4);
                glUniform2f(offsetLoc, 0.0f, 0.0f); glUniform2f(scaleLoc, 1.0f, 1.0f);

                int displayCount = std::min(VISIBLE, total - customLevelScroll);
                for (int i = 0; i < displayCount; ++i) {
                    int idx = customLevelScroll + i;
                    float buttonY = LIST_TOP - i * LBSTEP;

                    std::string rawName = std::filesystem::path(customLevelFiles[idx]).stem().string();
                    std::string displayName = rawName.length() > 20 ? rawName.substr(0, 19) + "." : rawName;

                    DrawButton(0.0f, buttonY, LIST_W, LBH, displayName, 0.042f, 0.2f, 0.8f, 1.0f, Hover(0.0f, buttonY, LIST_W, LBH));
                }

                bool canUp   = customLevelScroll > 0;
                bool canDown = customLevelScroll + VISIBLE < total;
                float arrowX = 0.82f;
                float arrowUpY   = LIST_TOP + 0.01f;
                float arrowDownY = LIST_TOP - (VISIBLE - 1) * LBSTEP - 0.01f;

                {
                    float bx = arrowX, by2 = arrowUpY, bw = 0.20f, bh = LBH;
                    bool hov = canUp && Hover(bx, by2, bw, bh);
                    glUniform2f(offsetLoc, bx, by2); glUniform2f(scaleLoc, bw, bh);
                    glBindVertexArray(quadVAO);
                    float rc = canUp ? 0.9f : 0.3f;
                    glUniform4f(colorLoc, rc*0.15f, rc*0.15f, rc*0.15f, 0.8f);
                    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                    glBindVertexArray(rectVAO);
                    glUniform4f(colorLoc, rc, rc, rc, hov ? 1.0f : 0.5f);
                    glLineWidth(hov ? 4.0f : 2.0f);
                    glDrawArrays(GL_LINE_LOOP, 0, 4);
                    glUniform2f(offsetLoc, 0.0f, 0.0f); glUniform2f(scaleLoc, 1.0f, 1.0f);
                    std::vector<float> upArr;
                    float as = 0.045f;
                    AddLine(upArr, 0.0f, -as, 0.0f,  as, bx, by2, 1.0f);
                    AddLine(upArr, 0.0f,  as, -as*0.6f, as*0.25f, bx, by2, 1.0f);
                    AddLine(upArr, 0.0f,  as,  as*0.6f, as*0.25f, bx, by2, 1.0f);
                    glLineWidth(hov ? 3.5f : 2.5f);
                    glUniform4f(colorLoc, rc, rc, rc, canUp ? 1.0f : 0.4f);
                    DrawDynamicLines(upArr);
                }
                {
                    float bx = arrowX, by2 = arrowDownY, bw = 0.20f, bh = LBH;
                    bool hov = canDown && Hover(bx, by2, bw, bh);
                    glUniform2f(offsetLoc, bx, by2); glUniform2f(scaleLoc, bw, bh);
                    glBindVertexArray(quadVAO);
                    float rc = canDown ? 0.9f : 0.3f;
                    glUniform4f(colorLoc, rc*0.15f, rc*0.15f, rc*0.15f, 0.8f);
                    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                    glBindVertexArray(rectVAO);
                    glUniform4f(colorLoc, rc, rc, rc, hov ? 1.0f : 0.5f);
                    glLineWidth(hov ? 4.0f : 2.0f);
                    glDrawArrays(GL_LINE_LOOP, 0, 4);
                    glUniform2f(offsetLoc, 0.0f, 0.0f); glUniform2f(scaleLoc, 1.0f, 1.0f);
                    std::vector<float> downArr;
                    float as2 = 0.045f;
                    AddLine(downArr, 0.0f,  as2, 0.0f, -as2, bx, by2, 1.0f);
                    AddLine(downArr, 0.0f, -as2, -as2*0.6f, -as2*0.25f, bx, by2, 1.0f);
                    AddLine(downArr, 0.0f, -as2,  as2*0.6f, -as2*0.25f, bx, by2, 1.0f);
                    glLineWidth(hov ? 3.5f : 2.5f);
                    glUniform4f(colorLoc, rc, rc, rc, canDown ? 1.0f : 0.4f);
                    DrawDynamicLines(downArr);
                }

                glLineWidth(1.5f); glUniform4f(colorLoc, 0.5f, 0.5f, 0.5f, 0.7f);
                std::string scrollInfo = std::to_string(customLevelScroll + 1) + "-" +
                    std::to_string(std::min(customLevelScroll + VISIBLE, total)) + "/" + std::to_string(total);
                float scrollInfoX = arrowX - 0.13f;
                DrawDynamicLines(GenerateText(scrollInfo, CenterX(scrollInfo, 0.028f, scrollInfoX), arrowDownY - LBH * 0.5f - 0.07f, 0.028f));
            }

            float bottomY = -0.62f;
            glLineWidth(1.5f); glUniform4f(colorLoc, 0.3f, 0.3f, 0.3f, 0.5f);
            { std::vector<float> sep; AddLine(sep, 0,0,1,0, -0.75f, bottomY + 0.22f, 1.50f); DrawDynamicLines(sep); }

            DrawButton(0.0f, bottomY,       1.30f, 0.210f, "IMPORT LEVEL", 0.054f, 0.2f, 1.0f, 0.4f, Hover(0.0f, bottomY,       1.30f, 0.210f));
            DrawButton(0.0f, bottomY-0.30f, 1.30f, 0.210f, "BACK TO MENU", 0.054f, 1.0f, 0.0f, 0.8f, Hover(0.0f, bottomY-0.30f, 1.30f, 0.210f));
        }

        else if (currentState == SETTINGS) {
            glUniform2f(offsetLoc,0.0f,0.0f); glUniform2f(scaleLoc,1.0f,1.0f);
            glLineWidth(5.0f);
            glUniform4f(colorLoc, 0.5f, 0.3f, 1.0f, 1.0f);
            DrawDynamicLines(GenerateText("SETTINGS", CenterX("SETTINGS", 0.095f), 0.88f, 0.095f));

            glLineWidth(1.5f); glUniform4f(colorLoc, 0.4f, 0.2f, 0.8f, 0.4f);
            { std::vector<float> sep; AddLine(sep, 0,0,1,0, -0.75f, 0.76f, 1.50f); DrawDynamicLines(sep); }

            glLineWidth(2.0f);
            glUniform4f(colorLoc, 0.7f, 0.8f, 1.0f, 1.0f);
            float scale = 0.046f;
            float ty = 0.64f; float tgap = 0.145f;
            DrawDynamicLines(GenerateText("G    CHANGE GRID SIZE", CenterX("G    CHANGE GRID SIZE", scale), ty, scale)); ty -= tgap;
            DrawDynamicLines(GenerateText("F1   WINDOW 600X600",   CenterX("F1   WINDOW 600X600",   scale), ty, scale)); ty -= tgap;
            DrawDynamicLines(GenerateText("F2   WINDOW 900X900",   CenterX("F2   WINDOW 900X900",   scale), ty, scale)); ty -= tgap;
            DrawDynamicLines(GenerateText("F3   WINDOW 1200X900",  CenterX("F3   WINDOW 1200X900",  scale), ty, scale)); ty -= tgap;
            DrawDynamicLines(GenerateText("F4   FULLSCREEN",       CenterX("F4   FULLSCREEN",       scale), ty, scale));

            glLineWidth(1.0f); glUniform4f(colorLoc, 0.3f, 0.3f, 0.4f, 0.4f);
            { std::vector<float> sep; AddLine(sep, 0,0,1,0, -0.75f, -0.07f, 1.50f); DrawDynamicLines(sep); }

            glLineWidth(2.0f);
            glUniform4f(colorLoc, 0.4f, 0.9f, 0.8f, 1.0f);
            std::string gs = "GRID: "; gs += std::to_string(gridCells); gs += "X"; gs += std::to_string(gridCells);
            DrawDynamicLines(GenerateText(gs, CenterX(gs, 0.058f), -0.18f, 0.058f));

            glLineWidth(1.0f); glUniform4f(colorLoc, 0.3f, 0.3f, 0.4f, 0.4f);
            { std::vector<float> sep; AddLine(sep, 0,0,1,0, -0.75f, -0.31f, 1.50f); DrawDynamicLines(sep); }

            
            glLineWidth(2.5f);
            glUniform4f(colorLoc, 0.8f, 0.6f, 1.0f, 1.0f);
            DrawDynamicLines(GenerateText("VOLUME", CenterX("VOLUME", 0.052f), -0.41f, 0.052f));

            
            for (int i = 0; i < 10; ++i) {
                float segX = -0.27f + i * 0.060f;
                float segY = -0.57f;
                bool lit = i < audioVolumeLevel;
                glUniform2f(offsetLoc, segX, segY); glUniform2f(scaleLoc, 0.046f, 0.11f);
                glBindVertexArray(quadVAO);
                glUniform4f(colorLoc, lit ? 0.3f : 0.1f, lit ? 0.9f : 0.2f, lit ? 0.6f : 0.2f, lit ? 0.9f : 0.4f);
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            }
            glUniform2f(offsetLoc, 0.0f, 0.0f); glUniform2f(scaleLoc, 1.0f, 1.0f);

            
            DrawButton(-0.50f, -0.57f, 0.18f, 0.14f, "-", 0.058f, 0.9f, 0.3f, 0.3f, Hover(-0.50f, -0.57f, 0.18f, 0.14f));
            DrawButton( 0.50f, -0.57f, 0.18f, 0.14f, "+", 0.058f, 0.3f, 0.9f, 0.3f, Hover( 0.50f, -0.57f, 0.18f, 0.14f));

            
            glLineWidth(2.0f); glUniform4f(colorLoc, 1.0f, 1.0f, 1.0f, 0.9f);
            std::string volStr = std::to_string(audioVolumeLevel);
            DrawDynamicLines(GenerateText(volStr, CenterX(volStr, 0.042f), -0.70f, 0.042f));

            
            glLineWidth(1.5f); glUniform4f(colorLoc, 0.5f, 0.5f, 0.6f, 0.7f);
            DrawDynamicLines(GenerateText("NUM+/-  volume anywhere", CenterX("NUM+/-  volume anywhere", 0.032f), -0.82f, 0.032f));

            DrawButton(0.0f, -0.94f, 1.20f, 0.19f, "MENU", 0.065f, 1.0f, 0.0f, 0.8f, Hover(0.0f, -0.94f, 1.20f, 0.19f));
        }

        else if (currentState == GAME) {
            glBindVertexArray(gridVAO);
            glUniform2f(offsetLoc,0.0f,0.0f); glUniform2f(scaleLoc,1.0f,1.0f);
            glUniform4f(colorLoc, 0.2f,0.1f,0.6f,0.6f);
            glDrawArrays(GL_LINES, 0, (GLsizei)(gridCoords.size()*4));

            float pulse = 0.8f + 0.2f * sinf(currentFrame * 6.0f);
            glBindVertexArray(quadVAO);
            glUniform2f(offsetLoc, cubeOffsetX, cubeOffsetY);
            glUniform2f(scaleLoc,  gridStep * 0.75f, gridStep * 0.75f);
            glUniform4f(colorLoc, 0.0f, pulse, pulse, 1.0f);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            for (const auto& atk : attacks) {
                glUniform2f(offsetLoc, atk.x, atk.y);
                if (atk.type == LASER || atk.type == LONG_LASER) {
                    if (atk.width > atk.height) glUniform2f(scaleLoc, atk.width, atk.active ? atk.height : 0.03f);
                    else                        glUniform2f(scaleLoc, atk.active ? atk.width : 0.03f, atk.height);
                } else if (atk.type == TILE_DMG) {
                    glUniform2f(scaleLoc, atk.active ? atk.width : atk.width * 0.7f, atk.active ? atk.height : atk.height * 0.7f);
                } else {
                    glUniform2f(scaleLoc, atk.width, atk.height);
                }
                glUniform4f(colorLoc, atk.r, atk.g, atk.b, atk.active?1.0f:0.4f);
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            }

            glUniform2f(offsetLoc,0.0f,0.0f); glUniform2f(scaleLoc,1.0f,1.0f);
            glLineWidth(2.5f);
            glUniform4f(colorLoc, 0.2f,0.9f,1.0f,0.9f);
            if (levelMode) {
                float remaining = currentLevel.duration - gameTime;
                char timeBuf[32]; snprintf(timeBuf, sizeof(timeBuf), "%.1f s", remaining);
                std::string ts(timeBuf);
                DrawDynamicLines(GenerateText(ts, CenterX(ts, 0.07f), 0.88f, 0.07f));
            } else {
                std::string sc = "SCORE: "; sc += std::to_string(score);
                DrawDynamicLines(GenerateText(sc, CenterX(sc, 0.07f), 0.88f, 0.07f));
            }
        }

        else if (currentState == GAME_OVER) {
            glUniform2f(offsetLoc,0.0f,0.0f); glUniform2f(scaleLoc,1.0f,1.0f);

            float blink = 0.6f + 0.4f * sinf(currentFrame * 3.0f);
            glLineWidth(7.0f);
            glUniform4f(colorLoc, 1.0f, 0.1f, 0.3f, blink * 0.35f);
            DrawDynamicLines(GenerateText("GAME OVER", CenterX("GAME OVER", 0.11f), 0.55f, 0.11f));
            glLineWidth(3.5f);
            glUniform4f(colorLoc, 1.0f, 0.2f, 0.4f, 1.0f);
            DrawDynamicLines(GenerateText("GAME OVER", CenterX("GAME OVER", 0.11f), 0.55f, 0.11f));

            glLineWidth(3.0f);
            glUniform4f(colorLoc, 0.0f,1.0f,1.0f,1.0f);
            std::string sc = "SCORE: "; sc += std::to_string(score);
            DrawDynamicLines(GenerateText(sc, CenterX(sc, 0.075f), 0.18f, 0.075f));

            glLineWidth(1.8f);
            glUniform4f(colorLoc, 0.6f,0.4f,1.0f,0.8f);
            DrawDynamicLines(GenerateText("R    RESTART", CenterX("R    RESTART", 0.050f), -0.10f, 0.050f));

            DrawButton(0.0f, -0.50f, 1.20f, 0.20f, "MENU", 0.065f, 1.0f, 0.0f, 0.8f, Hover(0.0f,-0.50f, 1.20f, 0.20f));
        }

        else if (currentState == LEVEL_COMPLETE) {
            glUniform2f(offsetLoc,0.0f,0.0f); glUniform2f(scaleLoc,1.0f,1.0f);

            float pulse = 0.7f + 0.3f * sinf(currentFrame * 2.5f);
            glLineWidth(7.0f);
            glUniform4f(colorLoc, 0.1f, 1.0f, 0.4f, pulse * 0.35f);
            DrawDynamicLines(GenerateText("COMPLETED", CenterX("COMPLETED", 0.11f), 0.55f, 0.11f));
            glLineWidth(3.5f);
            glUniform4f(colorLoc, 0.2f, 1.0f, 0.5f, 1.0f);
            DrawDynamicLines(GenerateText("COMPLETED", CenterX("COMPLETED", 0.11f), 0.55f, 0.11f));

            glLineWidth(2.0f);
            glUniform4f(colorLoc, 0.7f, 1.0f, 0.7f, 0.85f);
            DrawDynamicLines(GenerateText(currentLevel.name, CenterX(currentLevel.name, 0.055f), 0.20f, 0.055f));

            DrawButton(0.0f, -0.22f, 1.30f, 0.20f, "PLAY AGAIN", 0.058f, 0.2f, 1.0f, 0.4f, Hover(0.0f,-0.22f, 1.30f, 0.20f));
            DrawButton(0.0f, -0.50f, 1.30f, 0.20f, "LEVEL SELECT", 0.055f, 0.2f, 0.8f, 1.0f, Hover(0.0f,-0.50f, 1.30f, 0.20f));
            DrawButton(0.0f, -0.78f, 1.30f, 0.20f, "MENU", 0.065f, 1.0f, 0.0f, 0.8f, Hover(0.0f,-0.78f, 1.30f, 0.20f));
        }

        else if (currentState == LEVEL_EDITOR) {
            glUniform2f(vOffLoc, -0.50f, 0.53f); 
            glUniform2f(vSclLoc, 0.40f, 0.40f);

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
            glUniform2f(offsetLoc, cx, cy); glUniform2f(scaleLoc, gridStep * 1.1667f, gridStep * 1.1667f);
            glUniform4f(colorLoc, 0.0f, 1.0f, cursorPulse, 0.9f);
            glDrawArrays(GL_LINE_LOOP,0,4);

            std::vector<Attack> previewAttacks;
            if (!editorPlaying) {
                LevelEvent tempEvent = { editorSelectedType, editorTime, editorCursorCol, editorCursorRow, editorSelectedEdge };
                SpawnFromEvent(tempEvent, previewAttacks);
            }

            for (const auto& ev : currentLevel.events) {
                float lifeTime = (ev.type == LASER) ? 1.6f : ((ev.type == LONG_LASER) ? 4.6f : ((ev.type == TILE_DMG) ? 2.3f : 3.0f));
                if (editorTime >= ev.triggerTime && editorTime <= (ev.triggerTime + lifeTime)) {
                    std::vector<Attack> tmp;
                    SpawnFromEvent(ev, tmp);
                    if (!tmp.empty()) {
                        tmp[0].timer = (editorTime - ev.triggerTime);
                        if (tmp[0].type == NORMAL || tmp[0].type == BOOMERANG || tmp[0].type == MOVING_LASER || tmp[0].type == FAST_NORMAL || tmp[0].type == WIDE_NORMAL) {
                            float t = tmp[0].timer;
                            if (tmp[0].type == BOOMERANG) {
                                float dist = std::sqrt(tmp[0].dx*tmp[0].dx + tmp[0].dy*tmp[0].dy) * t;
                                if (dist > 1.8f) {
                                    float fullSpeedTime = 1.8f / std::sqrt(tmp[0].dx*tmp[0].dx + tmp[0].dy*tmp[0].dy);
                                    float returnTime = t - fullSpeedTime;
                                    tmp[0].x += tmp[0].dx * fullSpeedTime - tmp[0].dx * returnTime;
                                    tmp[0].y += tmp[0].dy * fullSpeedTime - tmp[0].dy * returnTime;
                                } else { tmp[0].x += tmp[0].dx * t; tmp[0].y += tmp[0].dy * t; }
                            } else { tmp[0].x += tmp[0].dx * t; tmp[0].y += tmp[0].dy * t; }
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
                    else                        glUniform2f(scaleLoc, atk.active ? atk.width : 0.03f, atk.height);
                } else if (atk.type == TILE_DMG) {
                    glUniform2f(scaleLoc, atk.active ? atk.width : atk.width * 0.7f, atk.active ? atk.height : atk.height * 0.7f);
                } else { glUniform2f(scaleLoc, atk.width, atk.height); }
                glUniform4f(colorLoc, atk.r, atk.g, atk.b, atk.active ? 0.7f : 0.3f);
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            }

            glUniform2f(vOffLoc, 0.0f, 0.0f);
            glUniform2f(vSclLoc, 1.0f, 1.0f);
            glUniform2f(offsetLoc,0.0f,0.0f); glUniform2f(scaleLoc,1.0f,1.0f);

            
            
            float colMid = 0.97f;        
            float leftX  = colMid - 0.33f; 

            
            glLineWidth(3.5f); glUniform4f(colorLoc, 0.0f, 0.9f, 0.5f, 1.0f);
            DrawDynamicLines(GenerateText("EDITOR", CenterX("EDITOR", 0.072f, colMid), 1.22f, 0.072f));

            glLineWidth(1.5f); glUniform4f(colorLoc, 0.0f, 0.6f, 0.3f, 0.4f);
            { std::vector<float> sep; AddLine(sep, 0,0,1,0, leftX, 1.12f, 0.66f); DrawDynamicLines(sep); }

            
            glLineWidth(1.8f); glUniform4f(colorLoc, 0.5f, 0.7f, 0.5f, 0.65f);
            DrawDynamicLines(GenerateText("NAME", leftX, 1.05f, 0.032f));

            {
                bool nameHov = Hover(colMid, 0.93f, 0.62f, 0.14f) && !levelNameEditing && !editorAudioEditing;
                glUniform2f(offsetLoc, colMid, 0.93f); glUniform2f(scaleLoc, 0.62f, 0.14f);
                glBindVertexArray(quadVAO);
                glUniform4f(colorLoc, levelNameEditing ? 0.02f : 0.01f, levelNameEditing ? 0.10f : 0.05f, 0.02f, 0.9f);
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                glBindVertexArray(rectVAO);
                glUniform4f(colorLoc,
                    levelNameEditing ? 0.2f : (nameHov ? 0.3f : 0.15f),
                    levelNameEditing ? 1.0f : (nameHov ? 0.9f : 0.6f),
                    levelNameEditing ? 0.4f : (nameHov ? 0.5f : 0.3f),
                    levelNameEditing ? 1.0f : (nameHov ? 0.9f : 0.6f));
                glLineWidth(levelNameEditing ? 2.5f : (nameHov ? 2.5f : 1.5f));
                glDrawArrays(GL_LINE_LOOP, 0, 4);
            }
            glUniform2f(offsetLoc,0.0f,0.0f); glUniform2f(scaleLoc,1.0f,1.0f);

            {
                float ns = 0.034f;
                std::string nameDisp = levelNameBuffer;
                while (!nameDisp.empty() && TextWidth(nameDisp, ns) > 0.56f) nameDisp = nameDisp.substr(1);
                bool nc = fmod((float)glfwGetTime(), 1.0f) < 0.5f;
                std::string nameWithCursor = nameDisp + (levelNameEditing && nc ? "|" : (levelNameBuffer.empty() ? "..." : ""));
                glLineWidth(2.0f);
                glUniform4f(colorLoc, levelNameEditing ? 0.4f : 0.5f, levelNameEditing ? 1.0f : 0.8f, levelNameEditing ? 0.6f : 0.5f, 1.0f);
                DrawDynamicLines(GenerateText(nameWithCursor, leftX + 0.02f, 0.93f - ns * 1.1f, ns));
            }

            glLineWidth(1.3f); glUniform4f(colorLoc, 0.35f, 0.35f, 0.35f, 0.6f);
            DrawDynamicLines(GenerateText(levelNameEditing ? "ENTER to confirm" : "click to edit",
                CenterX(levelNameEditing ? "ENTER to confirm" : "click to edit", 0.026f, colMid), 0.78f, 0.026f));

            
            glLineWidth(1.0f); glUniform4f(colorLoc, 0.2f, 0.4f, 0.2f, 0.3f);
            { std::vector<float> sep; AddLine(sep, 0,0,1,0, leftX, 0.72f, 0.66f); DrawDynamicLines(sep); }

            
            glLineWidth(1.8f); glUniform4f(colorLoc, 0.4f, 0.6f, 0.9f, 0.65f);
            DrawDynamicLines(GenerateText("AUDIO MP3", leftX, 0.67f, 0.032f));

            {
                bool audioHov = Hover(colMid, 0.55f, 0.62f, 0.14f) && !levelNameEditing && !editorAudioEditing;
                glUniform2f(offsetLoc, colMid, 0.55f); glUniform2f(scaleLoc, 0.62f, 0.14f);
                glBindVertexArray(quadVAO);
                glUniform4f(colorLoc, 0.01f, 0.02f, editorAudioEditing ? 0.10f : 0.04f, 0.9f);
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                glBindVertexArray(rectVAO);
                glUniform4f(colorLoc,
                    editorAudioEditing ? 0.2f : (audioHov ? 0.3f : 0.1f),
                    editorAudioEditing ? 0.5f : (audioHov ? 0.6f : 0.4f),
                    editorAudioEditing ? 1.0f : (audioHov ? 0.9f : 0.5f),
                    editorAudioEditing ? 1.0f : (audioHov ? 0.9f : 0.55f));
                glLineWidth(editorAudioEditing ? 2.5f : (audioHov ? 2.5f : 1.5f));
                glDrawArrays(GL_LINE_LOOP, 0, 4);
            }
            glUniform2f(offsetLoc,0.0f,0.0f); glUniform2f(scaleLoc,1.0f,1.0f);

            {
                float as = 0.028f;
                std::string audioDisp = editorAudioPath;
                while (!audioDisp.empty() && TextWidth(audioDisp, as) > 0.56f) audioDisp = audioDisp.substr(1);
                bool ac = fmod((float)glfwGetTime(), 1.0f) < 0.5f;
                std::string audioWithCursor = audioDisp + (editorAudioEditing && ac ? "|" : (editorAudioPath.empty() ? "none" : ""));
                glLineWidth(1.8f);
                glUniform4f(colorLoc, editorAudioEditing ? 0.5f : 0.4f, editorAudioEditing ? 0.8f : 0.6f, editorAudioEditing ? 1.0f : 0.8f, 1.0f);
                DrawDynamicLines(GenerateText(audioWithCursor, leftX + 0.02f, 0.55f - as * 1.1f, as));
            }

            glLineWidth(1.3f); glUniform4f(colorLoc, 0.3f, 0.3f, 0.4f, 0.6f);
            DrawDynamicLines(GenerateText(editorAudioEditing ? "ENTER to confirm" : "click to edit",
                CenterX(editorAudioEditing ? "ENTER to confirm" : "click to edit", 0.026f, colMid), 0.40f, 0.026f));

            
            glLineWidth(1.0f); glUniform4f(colorLoc, 0.2f, 0.3f, 0.5f, 0.3f);
            { std::vector<float> sep; AddLine(sep, 0,0,1,0, leftX, 0.34f, 0.66f); DrawDynamicLines(sep); }

            
            glLineWidth(2.5f); glUniform4f(colorLoc, 0.9f, 0.8f, 0.2f, 1.0f);
            std::string typeStr = "TYPE: " + std::string(GetAttackName(editorSelectedType));
            DrawDynamicLines(GenerateText(typeStr, CenterX(typeStr, 0.046f, colMid), 0.22f, 0.046f));

            
            glLineWidth(2.5f);
            glUniform4f(colorLoc, editorPlaying?0.2f:0.5f, editorPlaying?1.0f:0.5f, editorPlaying?0.2f:0.5f, 1.0f);
            const char* statusStr = editorPlaying ? "STATUS: PLAYING" : "STATUS: PAUSED";
            DrawDynamicLines(GenerateText(statusStr, CenterX(statusStr, 0.040f, colMid), 0.04f, 0.040f));

            
            glLineWidth(2.0f); glUniform4f(colorLoc, 1.0f, 1.0f, 0.0f, 0.9f);
            std::string edgeStr = "FROM: ";
            if (editorSelectedEdge == 0) edgeStr += "TOP";
            else if (editorSelectedEdge == 1) edgeStr += "BOTTOM";
            else if (editorSelectedEdge == 2) edgeStr += "LEFT";
            else edgeStr += "RIGHT";
            DrawDynamicLines(GenerateText(edgeStr, CenterX(edgeStr, 0.044f, colMid), -0.14f, 0.044f));

            
            std::vector<float> uiArrow;
            float ay = -0.30f; float arrowSize = 0.065f;
            if (editorSelectedEdge == 0) {
                AddLine(uiArrow, 0.0f,  arrowSize, 0.0f, -arrowSize, colMid, ay, 1.0f);
                AddLine(uiArrow, 0.0f, -arrowSize, -arrowSize*0.5f, -arrowSize*0.2f, colMid, ay, 1.0f);
                AddLine(uiArrow, 0.0f, -arrowSize,  arrowSize*0.5f, -arrowSize*0.2f, colMid, ay, 1.0f);
            } else if (editorSelectedEdge == 1) {
                AddLine(uiArrow, 0.0f, -arrowSize, 0.0f,  arrowSize, colMid, ay, 1.0f);
                AddLine(uiArrow, 0.0f,  arrowSize, -arrowSize*0.5f,  arrowSize*0.2f, colMid, ay, 1.0f);
                AddLine(uiArrow, 0.0f,  arrowSize,  arrowSize*0.5f,  arrowSize*0.2f, colMid, ay, 1.0f);
            } else if (editorSelectedEdge == 2) {
                AddLine(uiArrow, -arrowSize, 0.0f, arrowSize, 0.0f, colMid, ay, 1.0f);
                AddLine(uiArrow,  arrowSize, 0.0f, arrowSize*0.2f, -arrowSize*0.5f, colMid, ay, 1.0f);
                AddLine(uiArrow,  arrowSize, 0.0f, arrowSize*0.2f,  arrowSize*0.5f, colMid, ay, 1.0f);
            } else {
                AddLine(uiArrow,  arrowSize, 0.0f, -arrowSize, 0.0f, colMid, ay, 1.0f);
                AddLine(uiArrow, -arrowSize, 0.0f, -arrowSize*0.2f, -arrowSize*0.5f, colMid, ay, 1.0f);
                AddLine(uiArrow, -arrowSize, 0.0f, -arrowSize*0.2f,  arrowSize*0.5f, colMid, ay, 1.0f);
            }
            glLineWidth(3.0f); DrawDynamicLines(uiArrow);

            
            glLineWidth(1.0f); glUniform4f(colorLoc, 0.3f, 0.3f, 0.3f, 0.3f);
            { std::vector<float> sep; AddLine(sep, 0,0,1,0, leftX, -0.46f, 0.66f); DrawDynamicLines(sep); }

            
            std::string durStr = "MAX: " + std::to_string((int)currentLevel.duration) + "s";
            glLineWidth(2.0f); glUniform4f(colorLoc, 0.8f, 0.8f, 0.8f, 0.9f);
            DrawDynamicLines(GenerateText(durStr, CenterX(durStr, 0.042f, colMid), -0.56f, 0.042f));

            
            DrawButton(colMid - 0.22f, -0.72f, 0.18f, 0.14f, "-", 0.055f, 0.8f, 0.2f, 0.2f, Hover(colMid - 0.22f, -0.72f, 0.18f, 0.14f));
            DrawButton(colMid + 0.22f, -0.72f, 0.18f, 0.14f, "+", 0.055f, 0.2f, 0.8f, 0.2f, Hover(colMid + 0.22f, -0.72f, 0.18f, 0.14f));

            
            DrawButton(colMid, -0.92f, 0.52f, 0.16f, "MENU", 0.055f, 0.0f, 0.8f, 0.5f, Hover(colMid, -0.92f, 0.52f, 0.16f));

            
            
            float panelsY  = -0.40f; 
            float panelsH  = 0.62f; 
            float panelsW  = 1.24f; 
            float kScl     = 0.030f;
            float lineStep = 0.066f; 

            float ctrlX = -0.50f;
            {
                glUniform2f(offsetLoc, ctrlX, panelsY); glUniform2f(scaleLoc, panelsW, panelsH);
                glBindVertexArray(quadVAO); glUniform4f(colorLoc, 0.02f, 0.06f, 0.02f, 0.55f); glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                glBindVertexArray(rectVAO); glUniform4f(colorLoc, 0.15f, 0.35f, 0.15f, 0.45f); glLineWidth(1.5f); glDrawArrays(GL_LINE_LOOP, 0, 4);
                glUniform2f(offsetLoc, 0.0f, 0.0f); glUniform2f(scaleLoc, 1.0f, 1.0f);
            }
            glLineWidth(2.0f); glUniform4f(colorLoc, 0.5f, 0.85f, 0.5f, 0.9f);
            DrawDynamicLines(GenerateText("CONTROLS", CenterX("CONTROLS", kScl, ctrlX), panelsY + 0.24f, kScl));

            glUniform4f(colorLoc, 0.4f, 0.65f, 0.4f, 0.85f);
            float col1X = ctrlX - 0.54f; 
            float col2X = ctrlX + 0.08f;
            float startY = panelsY + 0.11f;

            
            DrawDynamicLines(GenerateText("SPC  add",    col1X, startY - 0*lineStep, kScl));
            DrawDynamicLines(GenerateText("BSP  del",    col1X, startY - 1*lineStep, kScl));
            DrawDynamicLines(GenerateText("Z+C  undo",   col1X, startY - 2*lineStep, kScl));
            DrawDynamicLines(GenerateText("DEL  clear",  col1X, startY - 3*lineStep, kScl));
            DrawDynamicLines(GenerateText("P    play",   col1X, startY - 4*lineStep, kScl));
            DrawDynamicLines(GenerateText("E    export", col1X, startY - 5*lineStep, kScl));

            
            DrawDynamicLines(GenerateText(",/.  time",   col2X, startY - 0*lineStep, kScl));
            DrawDynamicLines(GenerateText("arr  move",   col2X, startY - 1*lineStep, kScl));
            DrawDynamicLines(GenerateText("R    edge",   col2X, startY - 2*lineStep, kScl));
            DrawDynamicLines(GenerateText("1-8  type",   col2X, startY - 3*lineStep, kScl));
            DrawDynamicLines(GenerateText("F4   full",   col2X, startY - 4*lineStep, kScl));
            DrawDynamicLines(GenerateText("ESC  menu",   col2X, startY - 5*lineStep, kScl));

            float TL_START = -1.25f;
            float TL_END = 1.25f;
            float TL_Y = -1.06f;

            glLineWidth(2.0f); glUniform4f(colorLoc,1.0f,1.0f,1.0f,1.0f);
            char tBuf[32]; snprintf(tBuf, sizeof(tBuf), "TIME: %.1f s", editorTime);
            DrawDynamicLines(GenerateText(tBuf, TL_START, TL_Y + 0.13f, 0.055f));

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

            if (exportNotifTimer > 0.0f) {
                exportNotifTimer -= deltaTime;
                float alpha = std::min(1.0f, exportNotifTimer * 2.0f);
                bool isError = (exportNotifMsg == "EXPORT FAILED");
                glLineWidth(3.0f);
                glUniform4f(colorLoc, isError ? 1.0f : 0.2f, isError ? 0.2f : 1.0f, isError ? 0.2f : 0.4f, alpha);
                DrawDynamicLines(GenerateText(exportNotifMsg, CenterX(exportNotifMsg, 0.050f), -1.32f, 0.050f));
            }
        }

        else if (currentState == IMPORT_LEVEL) {
            glUniform2f(offsetLoc, 0.0f, 0.0f); glUniform2f(scaleLoc, 1.0f, 1.0f);

            glLineWidth(5.0f);
            glUniform4f(colorLoc, 0.2f, 1.0f, 0.4f, 1.0f);
            DrawDynamicLines(GenerateText("IMPORT LEVEL", CenterX("IMPORT LEVEL", 0.090f), 0.84f, 0.090f));

            glLineWidth(2.0f); glUniform4f(colorLoc, 0.2f, 1.0f, 0.4f, 0.35f);
            { std::vector<float> sep; AddLine(sep, 0,0,1,0, -0.75f, 0.68f, 1.50f); DrawDynamicLines(sep); }

            glLineWidth(2.0f); glUniform4f(colorLoc, 0.65f, 0.90f, 0.65f, 0.90f);
            DrawDynamicLines(GenerateText("Enter path to .txt level file:", CenterX("Enter path to .txt level file:", 0.040f), 0.57f, 0.040f));

            glLineWidth(1.5f); glUniform4f(colorLoc, 0.35f, 0.52f, 0.35f, 0.75f);
            DrawDynamicLines(GenerateText("e.g.  levels/my-level.txt", CenterX("e.g.  levels/my-level.txt", 0.034f), 0.44f, 0.034f));

            bool hasErr = !importErrorMsg.empty();
            {
                glUniform2f(offsetLoc, 0.0f, 0.25f); glUniform2f(scaleLoc, 1.70f, 0.160f);
                glBindVertexArray(quadVAO);
                glUniform4f(colorLoc, 0.01f, 0.04f, 0.01f, 0.95f);
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                glBindVertexArray(rectVAO);
                glUniform4f(colorLoc, hasErr ? 0.9f : 0.15f, hasErr ? 0.2f : 0.9f, hasErr ? 0.2f : 0.35f, hasErr ? 1.0f : 0.85f);
                glLineWidth(hasErr ? 3.0f : 2.0f);
                glDrawArrays(GL_LINE_LOOP, 0, 4);
            }
            glUniform2f(offsetLoc, 0.0f, 0.0f); glUniform2f(scaleLoc, 1.0f, 1.0f);

            {
                float pathScale = 0.038f;
                float maxPathW  = 1.58f;
                std::string displayPath = importPathBuffer;
                while (!displayPath.empty() && TextWidth(displayPath, pathScale) > maxPathW) displayPath = displayPath.substr(1);
                bool cursorVisible = fmod(currentFrame, 1.0f) < 0.5f;
                std::string displayWithCursor = displayPath + (cursorVisible ? "|" : " ");
                glLineWidth(2.2f);
                glUniform4f(colorLoc, 0.3f, 1.0f, 0.5f, 1.0f);
                DrawDynamicLines(GenerateText(displayWithCursor, -0.80f, 0.25f - pathScale * 1.05f, pathScale));
            }

            if (hasErr) {
                glUniform2f(offsetLoc, 0.0f, 0.05f); glUniform2f(scaleLoc, 1.70f, 0.140f);
                glBindVertexArray(quadVAO); glUniform4f(colorLoc, 0.15f, 0.02f, 0.02f, 0.92f); glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                glBindVertexArray(rectVAO); glUniform4f(colorLoc, 0.9f, 0.2f, 0.2f, 0.75f); glLineWidth(2.0f); glDrawArrays(GL_LINE_LOOP, 0, 4);
                glUniform2f(offsetLoc, 0.0f, 0.0f); glUniform2f(scaleLoc, 1.0f, 1.0f);
                glLineWidth(2.2f); glUniform4f(colorLoc, 1.0f, 0.4f, 0.4f, 1.0f);
                DrawDynamicLines(GenerateText(importErrorMsg, CenterX(importErrorMsg, 0.036f), 0.05f - 0.036f * 1.05f, 0.036f));
            }

            glUniform2f(offsetLoc, 0.0f, -0.32f); glUniform2f(scaleLoc, 2.20f, 0.340f);
            glBindVertexArray(quadVAO); glUniform4f(colorLoc, 0.02f, 0.05f, 0.02f, 0.65f); glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            glBindVertexArray(rectVAO); glUniform4f(colorLoc, 0.18f, 0.35f, 0.18f, 0.45f); glLineWidth(1.2f); glDrawArrays(GL_LINE_LOOP, 0, 4);
            glUniform2f(offsetLoc, 0.0f, 0.0f); glUniform2f(scaleLoc, 1.0f, 1.0f);

            glLineWidth(1.8f); glUniform4f(colorLoc, 0.42f, 0.65f, 0.42f, 0.85f);
            DrawDynamicLines(GenerateText("ENTER confirm   CTRL+V paste   CTRL+C copy", CenterX("ENTER confirm   CTRL+V paste   CTRL+C copy", 0.031f), -0.24f, 0.031f));
            DrawDynamicLines(GenerateText("BKSP delete   CTRL+A clear   ESC back",      CenterX("BKSP delete   CTRL+A clear   ESC back",      0.031f), -0.40f, 0.031f));

            DrawButton(0.0f, -0.74f, 1.34f, 0.200f, "BACK", 0.065f, 1.0f, 0.0f, 0.8f, Hover(0.0f, -0.74f, 1.34f, 0.200f));
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    
    AudioShutdown();
    glDeleteVertexArrays(1,&quadVAO); glDeleteBuffers(1,&quadVBO);
    glDeleteVertexArrays(1,&rectVAO); glDeleteBuffers(1,&rectVBO);
    glDeleteVertexArrays(1,&gridVAO); glDeleteBuffers(1,&gridVBO);
    glDeleteVertexArrays(1,&textVAO); glDeleteBuffers(1,&textVBO);
    glDeleteProgram(shaderProgram);
    glfwTerminate();
    return 0;
}