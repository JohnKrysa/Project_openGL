

/**
 * @file game.cpp
 * @brief Herní logika, správa levelů, kolize, GLFW callbacky a editor levelů.
 *
 * Modul zajišťuje:
 * - Spawn útoků v nekonečném módu (SpawnAttack) i z level eventů (SpawnFromEvent)
 * - Detekci kolize hráče s útoky (checkCollision)
 * - Funkce pro export/import levelů do/ze souborů
 * - Reset hry a nastavení mřížky
 * - Obsluhu vstupu (klávesnice, myš) pro všechny herní obrazovky
 */

#include "common.h"
#include "audio.h"

namespace fs = std::filesystem;

/**
 * @brief Exportuje aktuální level do textového souboru v adresáři levels/.
 *
 * Formát souboru:
 * @code
 * NAME  <název>
 * DURATION <sekundy>
 * AUDIO <cesta>          (volitelné)
 * <type> <triggerTime> <col> <row> <edge>
 * ...
 * @endcode
 *
 * Po úspěchu nastaví exportNotifMsg a exportNotifTimer (3 s zobrazení).
 */
void ExportLevel() {
    if (!fs::exists("levels")) {
        fs::create_directory("levels");
    }
    
    std::string safeName = levelNameBuffer;
    for (char& c : safeName) {
        if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|')
            c = '_';
    }
    if (safeName.empty()) safeName = "level_" + std::to_string((unsigned)time(NULL));
    
    std::string filename = "levels/" + safeName + ".txt";
    std::ofstream out(filename);
    
    if (out.is_open()) {
        out << "NAME " << safeName << "\n";
        out << "DURATION " << currentLevel.duration << "\n";
        
        if (!editorAudioPath.empty()) {
            out << "AUDIO " << editorAudioPath << "\n";
        }
        for (const auto& ev : currentLevel.events) {
            out << (int)ev.type << " " << ev.triggerTime << " " << ev.gridCol << " " << ev.gridRow << " " << ev.edge << "\n";
        }
        out.close();
        std::cout << "Level uspesne exportovan do: " << filename << "\n";
        exportNotifMsg = "EXPORTED: " + std::filesystem::path(filename).filename().string();
        exportNotifTimer = 3.0f;
    } else {
        std::cerr << "Chyba: Nepodarilo se zapsat soubor " << filename << "\n";
        exportNotifMsg = "EXPORT FAILED";
        exportNotifTimer = 3.0f;
    }
}

void SaveCustomLevelsIndex() {
    if (!fs::exists("levels")) fs::create_directory("levels");
    std::ofstream out("levels/index.txt");
    if (!out.is_open()) return;
    for (const auto& path : customLevelFiles) {
        out << path << "\n";
    }
}

void LoadCustomLevelsList() {
    customLevelFiles.clear();

    if (fs::exists("levels")) {
        for (const auto& entry : fs::directory_iterator("levels")) {
            if (entry.path().extension() == ".txt" && entry.path().filename() != "index.txt") {
                customLevelFiles.push_back(entry.path().string());
            }
        }
    }

    std::ifstream idx("levels/index.txt");
    if (!idx.is_open()) return;
    std::string line;
    while (std::getline(idx, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        if (fs::exists(line) &&
            std::find(customLevelFiles.begin(), customLevelFiles.end(), line) == customLevelFiles.end()) {
            customLevelFiles.push_back(line);
        }
    }
}

bool ImportLevel(const std::string& path) {
    std::string cleanPath = path;
    if (cleanPath.size() >= 2 && cleanPath.front() == '"' && cleanPath.back() == '"') {
        cleanPath = cleanPath.substr(1, cleanPath.size() - 2);
    }

    std::cout << "[IMPORT] Oteviram soubor: " << cleanPath << "\n";
    std::ifstream in(cleanPath);
    if (!in.is_open()) {
        std::cout << "[IMPORT] CHYBA: soubor nelze otevrit!\n";
        return false;
    }
    std::cout << "[IMPORT] Soubor otevren OK\n";
    
    currentLevel.events.clear();
    currentLevel.name.clear();
    currentLevel.audioPath.clear(); 

    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;

        std::istringstream ss(line);
        std::string token;
        if (!(ss >> token)) continue;

        if (token == "NAME") {
            std::string name;
            std::getline(ss, name);
            if (!name.empty() && name[0] == ' ') name = name.substr(1);
            currentLevel.name = name;
        } else if (token == "DURATION") {
            ss >> currentLevel.duration;
        } else if (token == "AUDIO") {
            
            std::string audioPath;
            std::getline(ss, audioPath);
            if (!audioPath.empty() && audioPath[0] == ' ') audioPath = audioPath.substr(1);
            currentLevel.audioPath = audioPath;
            std::cout << "[IMPORT] Audio: " << audioPath << "\n";
        } else {
            try {
                int type = std::stoi(token);
                float tTime; int col, row, edge;
                if (ss >> tTime >> col >> row >> edge) {
                    currentLevel.events.push_back({ (AttackType)type, tTime, col, row, edge });
                    std::cout << "[IMPORT] Event: type=" << type << " t=" << tTime << " col=" << col << " row=" << row << " edge=" << edge << "\n";
                } else {
                    std::cout << "[IMPORT] Radek nelze parsovat jako event: " << line << "\n";
                }
            } catch (...) {
                std::cout << "[IMPORT] Neznamy token: " << token << "\n";
            }
        }
    }
    in.close();
    std::cout << "[IMPORT] Celkem eventu: " << currentLevel.events.size() << ", name='" << currentLevel.name << "', duration=" << currentLevel.duration << "\n";
    
    std::sort(currentLevel.events.begin(), currentLevel.events.end(),
        [](const LevelEvent& a, const LevelEvent& b){ return a.triggerTime < b.triggerTime; });
        
    return true;
}

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
        case FAST_NORMAL: r=1.0f; g=0.5f; b=0.0f; break;
        case WIDE_NORMAL: r=0.8f; g=0.2f; b=1.0f; break;
        default:          r=1.0f; g=1.0f; b=1.0f; break;
    }
}

/**
 * @brief Spawne náhodný útok v nekonečném herním módu.
 *
 * Typ útoku se určuje náhodně (rand() % 16), přičemž pravděpodobnosti
 * jsou rovnoměrně rozděleny do 8 skupin po 2. Rychlost a velikost útoků
 * se škálují přes globalSpeedMultiplier a gridStep.
 *
 * Typy a jejich chování:
 * - NORMAL / BOOMERANG / FAST_NORMAL: letí ze strany, čtvercový hitbox
 * - LASER / LONG_LASER: statický pruh přes celé hřiště
 * - TILE_DMG: označí náhodnou dlaždici, poté zasáhne
 * - MOVING_LASER: laser pomalu projíždějící hřištěm
 * - WIDE_NORMAL: široký projektil pokrývající 3 dlaždice
 *
 * @param playerX Aktuální X pozice hráče (rezervováno pro budoucí homing útoky)
 * @param playerY Aktuální Y pozice hráče
 */
void SpawnAttack(float playerX, float playerY) {
    int   attackType = rand() % 16;
    float speed = 1.2f * globalSpeedMultiplier;

    int   edge    = rand() % 4;
    float spawnX  = 0.0f, spawnY = 0.0f, dx = 0.0f, dy = 0.0f;
    float randPos = gridCoords[rand() % gridCells];

    if      (edge == 0) { spawnX = randPos; spawnY =  1.4f; dy = -speed; }
    else if (edge == 1) { spawnX = randPos; spawnY = -1.4f; dy =  speed; }
    else if (edge == 2) { spawnX = -1.4f; spawnY = randPos; dx =  speed; }
    else                { spawnX =  1.4f; spawnY = randPos; dx = -speed; }

    bool isHorizontal = (edge == 0 || edge == 1);
    float sz = gridStep * 0.60f;   // menší než jeden node
    float lw = gridStep * 0.35f;   // šířka laseru (užší pruh)
    float tL = 2.8f;

    if (attackType < 3) {
        attacks.push_back({ NORMAL, spawnX, spawnY, dx, dy, 1.0f, 0.1f, 0.6f, 0,0,false,true, sz, sz });
    } else if (attackType < 5) {
        if (isHorizontal) attacks.push_back({ LASER, 0.0f, gridCoords[rand()%gridCells], 0,0, 0,1,1, 0,0,false,false, tL, lw });
        else              attacks.push_back({ LASER, gridCoords[rand()%gridCells], 0.0f, 0,0, 0,1,1, 0,0,false,false, lw, tL });
    } else if (attackType < 7) {
        attacks.push_back({ BOOMERANG, spawnX, spawnY, dx*1.2f, dy*1.2f, 1.0f,0.6f,0.1f, 0,0,false,true, sz, sz });
    } else if (attackType < 9) {
        if (isHorizontal) attacks.push_back({ LONG_LASER, 0.0f, gridCoords[rand()%gridCells], 0,0, 1,0.8f,0, 0,0,false,false, tL, lw });
        else              attacks.push_back({ LONG_LASER, gridCoords[rand()%gridCells], 0.0f, 0,0, 1,0.8f,0, 0,0,false,false, lw, tL });
    } else if (attackType < 11) {
        float rx = gridCoords[rand()%gridCells];
        float ry = gridCoords[rand()%gridCells];
        attacks.push_back({ TILE_DMG, rx, ry, 0,0, 1,0.2f,0.2f, 0,0,false,false, sz, sz });
    } else if (attackType < 13) {
        if (isHorizontal) attacks.push_back({ MOVING_LASER, 0.0f, spawnY, 0, dy*0.25f, 0.2f,1,0.2f, 0,0,false,true, tL, lw });
        else              attacks.push_back({ MOVING_LASER, spawnX, 0.0f, dx*0.25f, 0, 0.2f,1,0.2f, lw, tL });
    } else if (attackType < 15) {
        attacks.push_back({ FAST_NORMAL, spawnX, spawnY, dx*2.2f, dy*2.2f, 1.0f,0.5f,0.0f, 0,0,false,true, sz, sz });
    } else {
        float w = isHorizontal ? sz * 3.0f : sz;
        float h = isHorizontal ? sz : sz * 3.0f;
        attacks.push_back({ WIDE_NORMAL, spawnX, spawnY, dx*0.8f, dy*0.8f, 0.8f,0.2f,1.0f, 0,0,false,true, w, h });
    }
}

/**
 * @brief Spawne útok podle dat z LevelEvent (editor / level mód).
 *
 * Určí počáteční pozici a směr pohybu podle ev.edge a ev.gridCol/Row.
 * Pro laserové typy (LASER, LONG_LASER, MOVING_LASER) ignoruje col/row
 * a nastaví fixní šířku pokrývající celé hřiště.
 *
 * @param ev              Událost načtená z levelu
 * @param targetContainer Vektor, do kterého se nový útok přidá
 */
void SpawnFromEvent(const LevelEvent& ev, std::vector<Attack>& targetContainer) {
    float speed = 1.2f * globalSpeedMultiplier;
    float sx = 0, sy = 0, dx = 0, dy = 0;
    float tx = gridCoords[std::min(ev.gridCol, (int)gridCoords.size()-1)];
    float ty = gridCoords[std::min(ev.gridRow, (int)gridCoords.size()-1)];

    if      (ev.edge == 0) { sx = tx; sy =  1.4f; dy = -speed; }
    else if (ev.edge == 1) { sx = tx; sy = -1.4f; dy =  speed; }
    else if (ev.edge == 2) { sx = -1.4f; sy = ty; dx =  speed; }
    else if (ev.edge == 3) { sx =  1.4f; sy = ty; dx = -speed; }
    
    float sz = gridStep * 0.60f;
    float lw = gridStep * 0.35f;
    float w = sz, h = sz;
    
    if (ev.type == LASER || ev.type == LONG_LASER) {
        if (ev.edge == 0 || ev.edge == 1) { w = 2.8f; h = lw; sx = 0.0f; sy = ty; dx = 0; dy = 0; }
        else                              { w = lw; h = 2.8f; sx = tx; sy = 0.0f; dx = 0; dy = 0; }
    } 
    else if (ev.type == MOVING_LASER) {
        if (ev.edge == 0)      { sx = 0.0f; sy =  1.4f; dx = 0.0f;        dy = -speed * 0.25f; w = 2.8f; h = lw; }
        else if (ev.edge == 1) { sx = 0.0f; sy = -1.4f; dx = 0.0f;        dy =  speed * 0.25f; w = 2.8f; h = lw; }
        else if (ev.edge == 2) { sx = -1.4f; sy = 0.0f; dx =  speed * 0.25f; dy = 0.0f;        w = lw; h = 2.8f; }
        else                   { sx =  1.4f; sy = 0.0f; dx = -speed * 0.25f; dy = 0.0f;        w = lw; h = 2.8f; }
    } else if (ev.type == TILE_DMG) {
        w = sz; h = sz; sx = tx; sy = ty; dx = 0; dy = 0;
    } else if (ev.type == FAST_NORMAL) {
        w = sz; h = sz; dx *= 2.2f; dy *= 2.2f;
    } else if (ev.type == WIDE_NORMAL) {
        if (ev.edge == 0 || ev.edge == 1) { w = sz * 3.0f; h = sz; }
        else                              { w = sz; h = sz * 3.0f; }
        dx *= 0.8f; dy *= 0.8f;
    }
    
    float r = 1.0f, g = 1.0f, b = 1.0f;
    GetAttackColor(ev.type, r, g, b);

    bool active = true;
    if (ev.type == LASER || ev.type == LONG_LASER || ev.type == TILE_DMG) active = false;

    targetContainer.push_back({ ev.type, sx, sy, dx, dy, r, g, b, 0.0f, 0.0f, false, active, w, h });
}

/**
 * @brief AABB kolize hráče s obdélníkovým útokem.
 *
 * Hráč je považován za čtverec o straně gridStep * 0.75.
 * Malý epsilon (0.02) snižuje frustraci z hraničních kolizí.
 *
 * @param ax Střed útoku X
 * @param ay Střed útoku Y
 * @param aw Šířka útoku
 * @param ah Výška útoku
 * @return true pokud se hráč překrývá s útokem
 */
bool checkCollision(float ax, float ay, float aw, float ah) {
    return std::abs(cubeOffsetX - ax) < (gridStep / 2.0f + aw / 2.0f - 0.02f) &&
           std::abs(cubeOffsetY - ay) < (gridStep / 2.0f + ah / 2.0f - 0.02f);
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
                const float BH = 0.22f, BW = 1.35f, BSTEP = 0.28f;
                float by = 0.46f;
                if (IsHovered(0.0f, by, BW, BH)) {
                    ResetGame(window);
                    currentState = GAME;
                    levelMode = false;
                    
                    AudioStop(600);
                }
                by -= BSTEP;
                if (IsHovered(0.0f, by, BW, BH)) { currentState = CUSTOM_LEVELS; LoadCustomLevelsList(); customLevelScroll = 0; }
                by -= BSTEP;
                if (IsHovered(0.0f, by, BW, BH)) { currentState = SETTINGS; }
                by -= BSTEP;
                if (IsHovered(0.0f, by, BW, BH)) { currentState = LEVEL_EDITOR; AudioStop(400); }
                by -= BSTEP;
                if (IsHovered(0.0f, by, BW, BH)) { glfwSetWindowShouldClose(window, true); }

            } else if (currentState == SETTINGS) {
                if (IsHovered(0.0f, -0.94f, 1.20f, 0.19f)) {
                    currentState = MENU;
                    
                    AudioPlayLoop(menuMusicPath, (float)audioVolumeLevel / 10.0f);
                }
                
                if (IsHovered(-0.50f, -0.57f, 0.18f, 0.14f)) {
                    audioVolumeLevel = std::max(0, audioVolumeLevel - 1);
                    AudioSetVolume((float)audioVolumeLevel / 10.0f);
                }
                if (IsHovered(0.50f, -0.57f, 0.18f, 0.14f)) {
                    audioVolumeLevel = std::min(10, audioVolumeLevel + 1);
                    AudioSetVolume((float)audioVolumeLevel / 10.0f);
                }

            } else if (currentState == GAME_OVER) {
                if (IsHovered(0.0f, -0.50f, 1.20f, 0.20f)) {
                    currentState = MENU;
                    AudioPlayLoop(menuMusicPath, (float)audioVolumeLevel / 10.0f);
                }

            } else if (currentState == LEVEL_COMPLETE) {
                if (IsHovered(0.0f, -0.22f, 1.30f, 0.20f)) {
                    ResetGame(window);
                    currentState = GAME;
                    
                    if (!currentLevel.audioPath.empty())
                        AudioPlayLoop(currentLevel.audioPath, (float)audioVolumeLevel / 10.0f);
                    else
                        AudioStop(400);
                }
                if (IsHovered(0.0f, -0.50f, 1.30f, 0.20f)) {
                    currentState = CUSTOM_LEVELS;
                    LoadCustomLevelsList();
                    customLevelScroll = 0;
                    AudioPlayLoop(menuMusicPath, (float)audioVolumeLevel / 10.0f);
                }
                if (IsHovered(0.0f, -0.78f, 1.30f, 0.20f)) {
                    currentState = MENU;
                    AudioPlayLoop(menuMusicPath, (float)audioVolumeLevel / 10.0f);
                }

            } else if (currentState == IMPORT_LEVEL) {
                if (IsHovered(0.0f, -0.74f, 1.34f, 0.200f)) {
                    currentState = CUSTOM_LEVELS;
                    importPathBuffer.clear();
                    importErrorMsg.clear();
                }

            } else if (currentState == CUSTOM_LEVELS) {
                const float bottomY = -0.62f;
                if (IsHovered(0.0f, bottomY,        1.30f, 0.210f)) {
                    importPathBuffer.clear();
                    importErrorMsg.clear();
                    currentState = IMPORT_LEVEL;
                }
                if (IsHovered(0.0f, bottomY - 0.30f, 1.30f, 0.210f)) {
                    currentState = MENU;
                    AudioPlayLoop(menuMusicPath, (float)audioVolumeLevel / 10.0f);
                }

                if (!customLevelFiles.empty()) {
                    const float LBH = 0.185f, LBSTEP = 0.215f;
                    const float LIST_TOP = 0.55f;
                    const int   VISIBLE  = 5;
                    int total = (int)customLevelFiles.size();

                    float arrowX     = 0.82f;
                    float arrowUpY   = LIST_TOP + 0.01f;
                    float arrowDownY = LIST_TOP - (VISIBLE - 1) * LBSTEP - 0.01f;

                    if (IsHovered(arrowX, arrowUpY,   0.20f, LBH) && customLevelScroll > 0)
                        customLevelScroll--;
                    if (IsHovered(arrowX, arrowDownY, 0.20f, LBH) && customLevelScroll + VISIBLE < total)
                        customLevelScroll++;

                    int displayCount = std::min(VISIBLE, total - customLevelScroll);
                    for (int i = 0; i < displayCount; ++i) {
                        float buttonY = LIST_TOP - i * LBSTEP;
                        if (IsHovered(0.0f, buttonY, 1.30f, LBH)) {
                            if (ImportLevel(customLevelFiles[customLevelScroll + i])) {
                                ResetGame(window);
                                currentState = GAME;
                                levelMode = true;
                                
                                if (!currentLevel.audioPath.empty())
                                    AudioPlayLoop(currentLevel.audioPath, (float)audioVolumeLevel / 10.0f);
                                else
                                    AudioStop(400);
                            }
                        }
                    }
                }

            } else if (currentState == LEVEL_EDITOR) {
                const float colMid = 0.97f;

                if (IsHovered(colMid, -0.92f, 0.52f, 0.16f)) { currentState = MENU; AudioPlayLoop(menuMusicPath, (float)audioVolumeLevel / 10.0f); }

                
                if (IsHovered(colMid, 0.93f, 0.62f, 0.14f)) {
                    if (!editorAudioEditing) levelNameEditing = !levelNameEditing;
                }

                
                if (IsHovered(colMid, 0.55f, 0.62f, 0.14f)) {
                    if (!levelNameEditing) editorAudioEditing = !editorAudioEditing;
                }

                if (IsHovered(colMid - 0.22f, -0.72f, 0.18f, 0.14f)) {
                    currentLevel.duration = std::max(10.0f, currentLevel.duration - 10.0f);
                    editorTime = std::min(editorTime, currentLevel.duration);
                }
                if (IsHovered(colMid + 0.22f, -0.72f, 0.18f, 0.14f)) {
                    currentLevel.duration += 10.0f;
                }

                if (x >= -1.25f && x <= 1.25f && std::abs(y - (-1.15f)) < 0.12f) {
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
            ResetGame(window);
            currentState = GAME;
            
            if (levelMode && !currentLevel.audioPath.empty())
                AudioPlayLoop(currentLevel.audioPath, (float)audioVolumeLevel / 10.0f);
        }

        if (currentState == IMPORT_LEVEL) {
            if (key == GLFW_KEY_ESCAPE) {
                currentState = CUSTOM_LEVELS;
                importPathBuffer.clear();
                importErrorMsg.clear();
            }
            if (key == GLFW_KEY_BACKSPACE && !importPathBuffer.empty()) {
                while (!importPathBuffer.empty() &&
                       (importPathBuffer.back() & 0xC0) == 0x80) {
                    importPathBuffer.pop_back();
                }
                if (!importPathBuffer.empty())
                    importPathBuffer.pop_back();
                importErrorMsg.clear();
            }
            if (key == GLFW_KEY_BACKSLASH && !(mods & GLFW_MOD_CONTROL)) {
                importPathBuffer += (mods & GLFW_MOD_SHIFT) ? '|' : '\\';
                importErrorMsg.clear();
            }
            if (key == GLFW_KEY_SEMICOLON && !(mods & GLFW_MOD_CONTROL)) {
                importPathBuffer += (mods & GLFW_MOD_SHIFT) ? ':' : ';';
                importErrorMsg.clear();
            }
            if (key == GLFW_KEY_SPACE && !(mods & GLFW_MOD_CONTROL)) {
                importPathBuffer += ' ';
                importErrorMsg.clear();
            }
            if (key == GLFW_KEY_A && (mods & GLFW_MOD_CONTROL)) {
                importPathBuffer.clear();
                importErrorMsg.clear();
            }
            if (key == GLFW_KEY_C && (mods & GLFW_MOD_CONTROL)) {
                clipboard = importPathBuffer;
            }
            if (key == GLFW_KEY_V && (mods & GLFW_MOD_CONTROL)) {
                const char* sysClip = glfwGetClipboardString(window);
                if (sysClip) {
                    importPathBuffer += sysClip;
                } else {
                    importPathBuffer += clipboard;
                }
                importErrorMsg.clear();
            }
            if (key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER) {
                if (!importPathBuffer.empty()) {
                    if (ImportLevel(importPathBuffer)) {
                        std::string cleanPath = importPathBuffer;
                        if (cleanPath.size() >= 2 && cleanPath.front() == '"' && cleanPath.back() == '"')
                            cleanPath = cleanPath.substr(1, cleanPath.size() - 2);
                        if (std::find(customLevelFiles.begin(), customLevelFiles.end(), cleanPath) == customLevelFiles.end())
                            customLevelFiles.push_back(cleanPath);
                        SaveCustomLevelsIndex();
                        importPathBuffer.clear();
                        importErrorMsg.clear();
                        currentState = CUSTOM_LEVELS;
                    } else {
                        importErrorMsg = "FILE NOT FOUND OR INVALID";
                    }
                }
            }
        }

        if (currentState == LEVEL_EDITOR) {
            
            if (levelNameEditing) {
                if (key == GLFW_KEY_BACKSPACE && !levelNameBuffer.empty())
                    levelNameBuffer.pop_back();
                if (key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER || key == GLFW_KEY_ESCAPE)
                    levelNameEditing = false;
                if (key == GLFW_KEY_C && (mods & GLFW_MOD_CONTROL))
                    clipboard = levelNameBuffer;
                if (key == GLFW_KEY_V && (mods & GLFW_MOD_CONTROL)) {
                    for (char c : clipboard)
                        if (levelNameBuffer.size() < 32) levelNameBuffer += c;
                }
                if (key == GLFW_KEY_A && (mods & GLFW_MOD_CONTROL))
                    levelNameBuffer.clear();
                return;
            }

            
            if (editorAudioEditing) {
                if (key == GLFW_KEY_BACKSPACE && !editorAudioPath.empty()) {
                    while (!editorAudioPath.empty() && (editorAudioPath.back() & 0xC0) == 0x80)
                        editorAudioPath.pop_back();
                    if (!editorAudioPath.empty()) editorAudioPath.pop_back();
                }
                if (key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER || key == GLFW_KEY_ESCAPE)
                    editorAudioEditing = false;
                if (key == GLFW_KEY_A && (mods & GLFW_MOD_CONTROL))
                    editorAudioPath.clear();
                if (key == GLFW_KEY_V && (mods & GLFW_MOD_CONTROL)) {
                    const char* sysClip = glfwGetClipboardString(window);
                    if (sysClip) editorAudioPath += sysClip;
                    else editorAudioPath += clipboard;
                }
                if (key == GLFW_KEY_C && (mods & GLFW_MOD_CONTROL))
                    clipboard = editorAudioPath;
                
                if (key == GLFW_KEY_BACKSLASH && !(mods & GLFW_MOD_CONTROL))
                    editorAudioPath += (mods & GLFW_MOD_SHIFT) ? '|' : '\\';
                return;
            }

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
                undoStack.push_back(currentLevel.events);
                currentLevel.events.push_back({ editorSelectedType, editorTime, editorCursorCol, editorCursorRow, editorSelectedEdge });
                std::sort(currentLevel.events.begin(), currentLevel.events.end(),
                    [](const LevelEvent& a, const LevelEvent& b){ return a.triggerTime < b.triggerTime; });
                editorTime = std::min(currentLevel.duration, editorTime + 0.5f);
            }

            if (key == GLFW_KEY_BACKSPACE && !currentLevel.events.empty()) {
                undoStack.push_back(currentLevel.events);
                currentLevel.events.pop_back();
            }

            if (key == GLFW_KEY_1) editorSelectedType = NORMAL;
            if (key == GLFW_KEY_2) editorSelectedType = LASER;
            if (key == GLFW_KEY_3) editorSelectedType = BOOMERANG;
            if (key == GLFW_KEY_4) editorSelectedType = LONG_LASER;
            if (key == GLFW_KEY_5) editorSelectedType = TILE_DMG;
            if (key == GLFW_KEY_6) editorSelectedType = MOVING_LASER;
            if (key == GLFW_KEY_7) editorSelectedType = FAST_NORMAL;
            if (key == GLFW_KEY_8) editorSelectedType = WIDE_NORMAL;

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
                ExportLevel();
            }

            if (key == GLFW_KEY_DELETE && action == GLFW_PRESS) {
                undoStack.push_back(currentLevel.events);
                currentLevel.events.clear();
            }

            if (key == GLFW_KEY_Z && action == GLFW_PRESS && (mods & GLFW_MOD_CONTROL)) {
                if (!undoStack.empty()) {
                    currentLevel.events = undoStack.back();
                    undoStack.pop_back();
                }
            }
        }

        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            if (currentState == IMPORT_LEVEL) {
                currentState = CUSTOM_LEVELS;
                importPathBuffer.clear();
                importErrorMsg.clear();
            } else {
                currentState = MENU;
                AudioPlayLoop(menuMusicPath, (float)audioVolumeLevel / 10.0f);
            }
        }

        if (currentState == SETTINGS && key == GLFW_KEY_G && action == GLFW_PRESS) {
            int nextSize = gridCells + 2;
            if (nextSize > 15) nextSize = 5;
            SetGridSize(nextSize);
            UpdateGridVAO();
        }

        
        if (key == GLFW_KEY_KP_ADD && action == GLFW_PRESS) {
            audioVolumeLevel = std::min(10, audioVolumeLevel + 1);
            AudioSetVolume((float)audioVolumeLevel / 10.0f);
        }
        if (key == GLFW_KEY_KP_SUBTRACT && action == GLFW_PRESS) {
            audioVolumeLevel = std::max(0, audioVolumeLevel - 1);
            AudioSetVolume((float)audioVolumeLevel / 10.0f);
        }

        if (key == GLFW_KEY_F1 && action == GLFW_PRESS) { if (isFullscreen) ToggleFullscreen(window); glfwSetWindowSize(window, 600, 600); }
        if (key == GLFW_KEY_F2 && action == GLFW_PRESS) { if (isFullscreen) ToggleFullscreen(window); glfwSetWindowSize(window, 900, 900); }
        if (key == GLFW_KEY_F3 && action == GLFW_PRESS) { if (isFullscreen) ToggleFullscreen(window); glfwSetWindowSize(window, 1200, 900); }
        if (key == GLFW_KEY_F4 && action == GLFW_PRESS) { ToggleFullscreen(window); }
    }
}

void char_callback(GLFWwindow* window, unsigned int codepoint) {
    if (currentState == IMPORT_LEVEL) {
        if (codepoint < 0x80) {
            importPathBuffer += (char)codepoint;
        } else if (codepoint < 0x800) {
            importPathBuffer += (char)(0xC0 | (codepoint >> 6));
            importPathBuffer += (char)(0x80 | (codepoint & 0x3F));
        } else if (codepoint < 0x10000) {
            importPathBuffer += (char)(0xE0 | (codepoint >> 12));
            importPathBuffer += (char)(0x80 | ((codepoint >> 6) & 0x3F));
            importPathBuffer += (char)(0x80 | (codepoint & 0x3F));
        }
        importErrorMsg.clear();
    }
    if (currentState == LEVEL_EDITOR && levelNameEditing) {
        if (codepoint < 128 && levelNameBuffer.size() < 32) {
            levelNameBuffer += (char)codepoint;
        }
    }
    
    if (currentState == LEVEL_EDITOR && editorAudioEditing) {
        if (codepoint < 0x80) {
            editorAudioPath += (char)codepoint;
        } else if (codepoint < 0x800) {
            editorAudioPath += (char)(0xC0 | (codepoint >> 6));
            editorAudioPath += (char)(0x80 | (codepoint & 0x3F));
        } else if (codepoint < 0x10000) {
            editorAudioPath += (char)(0xE0 | (codepoint >> 12));
            editorAudioPath += (char)(0x80 | ((codepoint >> 6) & 0x3F));
            editorAudioPath += (char)(0x80 | (codepoint & 0x3F));
        }
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
        case FAST_NORMAL: return "FAST-N";
        case WIDE_NORMAL: return "WIDE-N";
        default:          return "UNKNOWN";
    }
}
