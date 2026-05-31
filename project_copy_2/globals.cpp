/**
 * @file globals.cpp
 * @brief Definice globálních proměnných sdílených přes celou hru.
 *
 * Všechny proměnné jsou deklarovány jako extern v common.h.
 * Tento soubor obsahuje jejich jedinou definici (alokaci paměti).
 */

#include "common.h"

/** @brief Aktuální herní stav; na začátku MENU. */
GameState currentState = MENU;

/** @brief Vybraný typ útoku v editoru levelů. */
AttackType editorSelectedType = NORMAL;

/** @brief Interpolovaná X pozice hráče (plynulý pohyb). */
float cubeOffsetX = 0.0f;
/** @brief Interpolovaná Y pozice hráče (plynulý pohyb). */
float cubeOffsetY = 0.0f;
/** @brief Cílová X pozice hráče (střed aktuální buňky mřížky). */
float targetOffsetX = 0.0f;
/** @brief Cílová Y pozice hráče (střed aktuální buňky mřížky). */
float targetOffsetY = 0.0f;

/** @brief Počet buněk mřížky na jednu osu (výchozí 5×5). */
int gridCells = 5;
/** @brief Souřadnice středů buněk mřížky; přepočítává SetGridSize(). */
std::vector<float> gridCoords = { -0.8f, -0.4f, 0.0f, 0.4f, 0.8f };
/** @brief Maximální absolutní souřadnice mřížky (krajní buňky). */
float gridLimit = 0.8f;
/** @brief Vzdálenost mezi sousedními buňkami mřížky. */
float gridStep  = 0.4f;

/** @brief Všechny aktivní útoky aktuálně ve scéně. */
std::vector<Attack> attacks;

/** @brief Čas od začátku aktuální herní session (sekundy). */
float gameTime            = 0.0f;
/** @brief Čas posledního spawnu útoku v nekonečném módu. */
float lastAttackSpawn     = 0.0f;
/** @brief Aktuální interval mezi spawny (zkracuje se s časem). */
float currentSpawnDelay   = 1.2f;
/** @brief Multiplikátor rychlosti všech útoků (roste s časem hry). */
float globalSpeedMultiplier = 1.0f;
/** @brief Skóre hráče; v nekonečném módu = gameTime * 10. */
int   score               = 0;

/** @brief Data aktuálně načteného levelu (název, eventy, délka, audio). */
Level        currentLevel;
/** @brief Sloupec kurzoru v editoru (0 = levý kraj). */
int          editorCursorCol      = 0;
/** @brief Řádek kurzoru v editoru (0 = spodní kraj). */
int          editorCursorRow      = 0;
/** @brief Aktuální čas na časové ose editoru (sekundy). */
float        editorTime           = 0.0f;
/** @brief Vybraná hrana spawnu v editoru: 0=shora, 1=zdola, 2=zleva, 3=zprava. */
int          editorSelectedEdge   = 0;
/** @brief Zda editor přehrává preview levelu. */
bool         editorPlaying        = false;
/** @brief Absolutní čas (glfwGetTime) začátku preview přehrávání. */
float        editorPlayStart      = 0.0f;
/** @brief Index příštího eventu ke spawnu během preview. */
size_t       editorNextEvent      = 0;
/** @brief True = hra probíhá jako level; false = nekonečný mód. */
bool         levelMode            = false;
/** @brief True pokud uživatel táhne kurzor myší po časové ose editoru. */
bool         isDraggingTimeline   = false;

/** @brief Zda je okno aktuálně v režimu fullscreen. */
bool isFullscreen  = false;
/** @brief Uložená šířka okna před přechodem do fullscreenu. */
int  savedWidth    = 900;
/** @brief Uložená výška okna před přechodem do fullscreenu. */
int  savedHeight   = 900;
/** @brief Uložená X pozice okna před přechodem do fullscreenu. */
int  savedX        = 100;
/** @brief Uložená Y pozice okna před přechodem do fullscreenu. */
int  savedY        = 100;

/** @brief OpenGL VAO pro vykreslení mřížky. */
unsigned int gridVAO;
/** @brief OpenGL VBO pro data mřížky. */
unsigned int gridVBO;
/** @brief OpenGL VAO pro dynamický vektorový text. */
unsigned int textVAO;
/** @brief OpenGL VBO pro dynamický vektorový text. */
unsigned int textVBO;

/** @brief Zásobník stavů pro operaci undo v editoru. */
std::vector<std::vector<LevelEvent>> undoStack;

/** @brief Interní schránka (clipboard) pro kopírování textu v editoru. */
std::string clipboard;

/** @brief Seznam cest k nalezeným souborům vlastních levelů. */
std::vector<std::string> customLevelFiles;
/** @brief Aktuální posun scrollu v seznamu vlastních levelů. */
int customLevelScroll = 0;

/** @brief Zpráva zobrazovaná po úspěšném/neúspěšném exportu levelu. */
std::string exportNotifMsg;
/** @brief Zbývající čas zobrazení exportní notifikace (sekundy). */
float exportNotifTimer = 0.0f;

/** @brief Buffer pro zadávání cesty při importu levelu. */
std::string importPathBuffer;
/** @brief Chybová zpráva zobrazená při neúspěšném importu. */
std::string importErrorMsg;

/** @brief Buffer pro editaci názvu levelu v editoru. */
std::string levelNameBuffer;
/** @brief True pokud je aktivní textový vstup pro název levelu. */
bool        levelNameEditing = false;

/** @brief Buffer pro editaci cesty k audio souboru levelu. */
std::string editorAudioPath;
/** @brief True pokud je aktivní textový vstup pro audio cestu. */
bool        editorAudioEditing = false;
/** @brief Hlasitost hudby v rozsahu 0–10 (zobrazuje se v Settings). */
int         audioVolumeLevel   = 7;
