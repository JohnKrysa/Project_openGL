/**
 * @file common.h
 * @brief Společné hlavičky, datové typy a deklarace pro celou hru Grid Dodge.
 *
 * Obsahuje výčty herních stavů a typů útoků, struktury Attack, LevelEvent a Level,
 * globální proměnné sdílené přes všechny moduly a deklarace všech veřejných funkcí.
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
#include <fstream>
#include <sstream>
#include <filesystem>

/**
 * @brief Výčet herních stavů určující, která obrazovka se právě zobrazuje.
 */
enum GameState {
    MENU,           ///< Hlavní menu
    SETTINGS,       ///< Nastavení
    GAME,           ///< Aktivní hra
    GAME_OVER,      ///< Obrazovka konce hry
    LEVEL_COMPLETE, ///< Dokončení levelu
    LEVEL_EDITOR,   ///< Editor levelů
    CUSTOM_LEVELS,  ///< Seznam vlastních levelů
    IMPORT_LEVEL    ///< Import levelu ze souboru
};

/**
 * @brief Výčet typů útoků, které mohou letět na hráče.
 */
enum AttackType {
    NORMAL,       ///< Standardní čtvercový projektil
    LASER,        ///< Statický laser pokrývající celý řádek/sloupec
    BOOMERANG,    ///< Projektil, který se vrací zpět
    LONG_LASER,   ///< Delší varianta laseru
    TILE_DMG,     ///< Útok poškozující konkrétní dlaždici
    MOVING_LASER, ///< Laser pohybující se přes mřížku
    FAST_NORMAL,  ///< Rychlá varianta normálního projektilu
    WIDE_NORMAL   ///< Široký projektil pokrývající 3 dlaždice
};

/**
 * @brief Hrana, ze které útok vylétá / spawne se.
 *
 * Místo holých čísel (0–3) se používá tento enum, aby byl kód
 * čitelnější a kompilátor mohl upozornit na chybějící case větve.
 */
enum SpawnEdge {
    EDGE_TOP    = 0, ///< Útok letí shora dolů
    EDGE_BOTTOM = 1, ///< Útok letí zdola nahoru
    EDGE_LEFT   = 2, ///< Útok letí zleva doprava
    EDGE_RIGHT  = 3  ///< Útok letí zprava doleva
};

/**
 * @brief Reprezentuje jeden aktivní útok ve hře.
 */
struct Attack {
    AttackType type;            ///< Typ útoku
    float x, y;                 ///< Aktuální pozice středu útoku
    float dx, dy;               ///< Rychlost pohybu (jednotky za sekundu)
    float r, g, b;              ///< Barva útoku (RGB, rozsah 0–1)
    float timer           = 0.0f; ///< Čas od vytvoření útoku (sekundy)
    float distanceTravelled = 0.0f; ///< Celková ujetá vzdálenost (pro MOVING_LASER)
    bool  returning       = false; ///< Zda se BOOMERANG vrací zpět
    bool  active          = true;  ///< Zda je útok aktivní (false = jen vizuální varování)
    float width           = 0.3f; ///< Šířka hitboxu útoku
    float height          = 0.3f; ///< Výška hitboxu útoku
};

/**
 * @brief Jedna naplánovaná událost v levelu (kdy a odkud vyletí útok).
 */
struct LevelEvent {
    AttackType type;    ///< Typ útoku
    float triggerTime;  ///< Čas v sekundách od začátku levelu
    int   gridCol;      ///< Sloupec mřížky (0 = levý)
    int   gridRow;      ///< Řádek mřížky (0 = spodní)
    int   edge;         ///< Hrana spawnu; viz SpawnEdge (0=shora, 1=zdola, 2=zleva, 3=zprava)
};

/**
 * @brief Kompletní data jednoho levelu.
 */
struct Level {
    std::string             name;      ///< Název levelu
    std::vector<LevelEvent> events;    ///< Seznam všech naplánovaných událostí
    float                   duration = 60.0f; ///< Délka levelu v sekundách
    std::string             audioPath; ///< Cesta k hudebnímu souboru levelu
};

// ---------------------------------------------------------------------------
// Globální herní proměnné
// ---------------------------------------------------------------------------

extern GameState  currentState;        ///< Aktuální herní stav
extern AttackType editorSelectedType;  ///< Vybraný typ útoku v editoru

extern float cubeOffsetX;   ///< Aktuální X pozice hráče (interpolovaná)
extern float cubeOffsetY;   ///< Aktuální Y pozice hráče (interpolovaná)
extern float targetOffsetX; ///< Cílová X pozice hráče (na mřížce)
extern float targetOffsetY; ///< Cílová Y pozice hráče (na mřížce)

extern int                gridCells;  ///< Počet buněk mřížky na jednu osu
extern std::vector<float> gridCoords; ///< Souřadnice středů buněk mřížky
extern float              gridLimit;  ///< Krajní souřadnice mřížky (±gridLimit)
extern float              gridStep;   ///< Vzdálenost mezi sousedními buňkami

extern std::vector<Attack> attacks; ///< Všechny aktivní útoky ve scéně

extern float gameTime;              ///< Čas od začátku aktuální herní session (sekundy)
extern float lastAttackSpawn;       ///< Čas posledního spawnu útoku
extern float currentSpawnDelay;     ///< Aktuální interval mezi spawny útoků
extern float globalSpeedMultiplier; ///< Globální multiplikátor rychlosti útoků
extern int   score;                 ///< Aktuální skóre hráče

extern Level   currentLevel;         ///< Data aktuálně načteného levelu
extern int     editorCursorCol;      ///< Kurzor editoru – sloupec
extern int     editorCursorRow;      ///< Kurzor editoru – řádek
extern float   editorTime;           ///< Aktuální čas na časové ose editoru
extern int     editorSelectedEdge;   ///< Vybraná hrana spawnu v editoru (viz SpawnEdge)
extern bool    editorPlaying;        ///< Zda editor přehrává preview levelu
extern float   editorPlayStart;      ///< Časová značka začátku přehrávání v editoru
extern size_t  editorNextEvent;      ///< Index příštího eventu ke spawnu při preview
extern bool    levelMode;            ///< True = hra běží jako level, false = nekonečný mód
extern bool    isDraggingTimeline;   ///< Zda uživatel táhne kurzor na časové ose

extern bool isFullscreen;            ///< Zda je okno v režimu fullscreen
extern int  savedWidth, savedHeight; ///< Uložená velikost okna před fullscreenem
extern int  savedX, savedY;          ///< Uložená pozice okna před fullscreenem

extern unsigned int gridVAO, gridVBO; ///< OpenGL buffery pro mřížku
extern unsigned int textVAO, textVBO; ///< OpenGL buffery pro vektorový text

extern std::string vertexShaderSource;   ///< Zdrojový kód vertex shaderu
extern std::string fragmentShaderSource; ///< Zdrojový kód fragment shaderu

extern std::vector<std::vector<LevelEvent>> undoStack; ///< Zásobník pro undo v editoru

extern std::string clipboard; ///< Interní schránka pro kopírování textu

extern std::vector<std::string> customLevelFiles; ///< Cesty k nalzeným souborům levelů
extern int                      customLevelScroll; ///< Posun scrollu v seznamu levelů

extern std::string exportNotifMsg;   ///< Zpráva notifikace po exportu levelu
extern float       exportNotifTimer; ///< Zbývající čas zobrazení notifikace (sekundy)

extern std::string importPathBuffer; ///< Buffer pro zadávání cesty při importu
extern std::string importErrorMsg;   ///< Chybová zpráva při neúspěšném importu

extern std::string levelNameBuffer;  ///< Buffer pro editaci názvu levelu
extern bool        levelNameEditing; ///< Zda je aktivní textový vstup pro název levelu

extern std::string editorAudioPath;    ///< Cesta k audio souboru zadaná v editoru
extern bool        editorAudioEditing; ///< Zda je aktivní textový vstup pro audio cestu
extern int         audioVolumeLevel;   ///< Hlasitost (0–10)

// ---------------------------------------------------------------------------
// Herní funkce
// ---------------------------------------------------------------------------

/**
 * @brief Nastaví velikost mřížky a přepočítá souřadnice buněk.
 * @param size Počet buněk na jednu osu (musí být liché číslo, např. 5, 7, 9…)
 */
void SetGridSize(int size);

/**
 * @brief Překreslí OpenGL VAO mřížky podle aktuálních gridCoords.
 */
void UpdateGridVAO();

/**
 * @brief Resetuje herní stav do výchozí polohy (čistí útoky, skóre, pozici hráče).
 * @param window Ukazatel na GLFW okno (pro reset titulku)
 */
void ResetGame(GLFWwindow* window);

/**
 * @brief Vrátí RGB barvu odpovídající danému typu útoku.
 * @param t    Typ útoku
 * @param r    [out] Červená složka (0–1)
 * @param g    [out] Zelená složka (0–1)
 * @param b    [out] Modrá složka (0–1)
 */
void GetAttackColor(AttackType t, float& r, float& g, float& b);

/**
 * @brief Spawne náhodný útok v nekonečném módu.
 * @param playerX Aktuální X pozice hráče (pro budoucí homing útoky)
 * @param playerY Aktuální Y pozice hráče
 */
void SpawnAttack(float playerX, float playerY);

/**
 * @brief Spawne útok podle dat z LevelEvent.
 * @param ev              Událost z levelu (typ, čas, pozice, hrana)
 * @param targetContainer Vektor, do kterého se útok přidá
 */
void SpawnFromEvent(const LevelEvent& ev, std::vector<Attack>& targetContainer);

/**
 * @brief Zkontroluje kolizi hráče s daným obdélníkem.
 * @param ax Střed obdélníku X
 * @param ay Střed obdélníku Y
 * @param aw Šířka obdélníku
 * @param ah Výška obdélníku
 * @return true pokud nastala kolize, jinak false
 */
bool checkCollision(float ax, float ay, float aw, float ah);

/**
 * @brief Vrátí čitelný název typu útoku (pro UI editoru).
 * @param t Typ útoku
 * @return Krátký řetězec, např. "NORMAL", "LASER"
 */
const char* GetAttackName(AttackType t);

/**
 * @brief Aktualizuje pozici, timer a stav jednoho útoku pro aktuální snímek.
 *
 * Pohybuje útokem, přepíná active flag u laserů/tile a detekuje kolizi s hráčem.
 * Při kolizi nastaví currentState = GAME_OVER a zavolá AudioStop(800).
 *
 * @param atk       Útok ke zpracování (upravován in-place)
 * @param deltaTime Čas od posledního snímku (sekundy)
 */
void UpdateAttack(Attack& atk, float deltaTime);

/**
 * @brief Rozhodne, zda má být útok odstraněn ze scény.
 *
 * Útok je odstraněn pokud vyletí mimo hranice hřiště nebo vyprší jeho životnost.
 *
 * @param atk Útok ke kontrole
 * @return true pokud má být útok odstraněn
 */
bool ShouldRemoveAttack(const Attack& atk);

// ---------------------------------------------------------------------------
// Renderovací / shader funkce
// ---------------------------------------------------------------------------

/**
 * @brief Zkompiluje a slinkuje vertex + fragment shader do programu.
 * @param vs Zdrojový kód vertex shaderu
 * @param fs Zdrojový kód fragment shaderu
 * @return OpenGL ID shader programu
 */
unsigned int CreateShader(const std::string& vs, const std::string& fs);

/**
 * @brief Přidá dvojici vrcholů (úsečku) do vektoru pro vektorový text.
 * @param v  Cílový vektor vrcholů
 * @param x1,y1 Počáteční bod (v normalizovaném prostoru písmena)
 * @param x2,y2 Koncový bod
 * @param ox,oy Offset pozice písmena ve světových souřadnicích
 * @param s  Měřítko písmena
 */
void AddLine(std::vector<float>& v, float x1, float y1, float x2, float y2,
             float ox, float oy, float s);

/**
 * @brief Vypočítá šířku řetězce v daném měřítku.
 * @param str   Vstupní řetězec
 * @param scale Měřítko textu
 * @return Šířka v souřadnicích scény
 */
float TextWidth(const std::string& str, float scale);

/**
 * @brief Vypočítá X souřadnici tak, aby byl text horizontálně vycentrován.
 * @param str     Vstupní řetězec
 * @param scale   Měřítko textu
 * @param centerX X souřadnice středu (default 0)
 * @return Levá X souřadnice pro zahájení vykreslení
 */
float CenterX(const std::string& str, float scale, float centerX = 0.0f);

/**
 * @brief Vygeneruje seznam vrcholů pro vykreslení vektorového textu.
 * @param str    Řetězec k vykreslení (podporuje A–Z, a–z, 0–9 a základní znaky)
 * @param startX Levá X souřadnice prvního písmene
 * @param startY Dolní Y souřadnice textu
 * @param scale  Měřítko textu
 * @return Vektor vrcholů (formát XYZ, kreslí se jako GL_LINES)
 */
std::vector<float> GenerateText(const std::string& str, float startX,
                                float startY, float scale);

/**
 * @brief Nahraje vrcholy do textVBO a vykreslí je jako úsečky.
 * @param pts Vektor vrcholů vygenerovaný funkcí GenerateText()
 */
void DrawDynamicLines(const std::vector<float>& pts);

// ---------------------------------------------------------------------------
// GLFW callbacky
// ---------------------------------------------------------------------------

/** @brief Callback při změně velikosti framebufferu. */
void framebuffer_size_callback(GLFWwindow* window, int width, int height);

/** @brief Callback pro stisk/uvolnění tlačítka myši. */
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

/** @brief Callback pro stisk/uvolnění klávesy. */
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

/** @brief Callback pro zadání znaku (Unicode). */
void char_callback(GLFWwindow* window, unsigned int codepoint);

/** @brief Přepne okno mezi fullscreen a oknovým režimem. */
void ToggleFullscreen(GLFWwindow* window);

// ---------------------------------------------------------------------------
// Level management
// ---------------------------------------------------------------------------

/** @brief Exportuje aktuální level do souboru v adresáři levels/. */
void ExportLevel();

/** @brief Načte seznam dostupných levelů z adresáře levels/. */
void LoadCustomLevelsList();

/** @brief Uloží index vlastních levelů do levels/index.txt. */
void SaveCustomLevelsIndex();

/**
 * @brief Importuje level ze zadaného souboru do currentLevel.
 * @param path Cesta k souboru levelu (.txt)
 * @return true při úspěchu, false při chybě čtení
 */
bool ImportLevel(const std::string& path);
