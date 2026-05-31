/**
 * @file audio.h
 * @brief Rozhraní audio systému postaveného na knihovně miniaudio.
 *
 * Poskytuje funkce pro inicializaci enginu, přehrávání hudby ve smyčce,
 * plynulé ztlumení (fade-out) a řízení hlasitosti.
 * Interně je spravován jeden aktivní zvukový stream (s_sound).
 */

#pragma once
#include <string>

/**
 * @brief Inicializuje audio engine (miniaudio, stereo 44100 Hz).
 * @return true při úspěchu, false pokud inicializace selže
 */
bool AudioInit();

/**
 * @brief Uvolní všechny audio zdroje a vypne engine.
 *
 * Bezpečné volat i bez předchozího úspěšného AudioInit().
 */
void AudioShutdown();

/**
 * @brief Načte audio soubor a spustí ho ve smyčce.
 *
 * Pokud již nějaký zvuk hraje, nejprve ho zastaví a uvolní.
 * Podporuje uvozovky okolo cesty (jsou automaticky oříznuty).
 *
 * @param path   Cesta k audio souboru (MP3, WAV, FLAC, …)
 * @param volume Počáteční hlasitost v rozsahu 0.0–1.0 (výchozí 0.7)
 */
void AudioPlayLoop(const std::string& path, float volume = 0.7f);

/**
 * @brief Zastaví přehrávání, volitelně s plynulým fade-out.
 *
 * Při fadeMs <= 0 dojde k okamžitému zastavení.
 * Fade je průběžně zpracováván v AudioUpdate().
 *
 * @param fadeMs Délka fade-out v milisekundách (výchozí 400 ms)
 */
void AudioStop(int fadeMs = 400);

/**
 * @brief Nastaví hlasitost aktuálně přehrávaného zvuku.
 * @param volume Hlasitost v rozsahu 0.0–1.0 (hodnoty mimo rozsah jsou oříznuty)
 */
void AudioSetVolume(float volume);

/**
 * @brief Vrátí aktuálně nastavenou hlasitost.
 * @return Hlasitost v rozsahu 0.0–1.0
 */
float AudioGetVolume();

/**
 * @brief Zjistí, zda zvuk právě hraje.
 * @return true pokud zvuk aktivně přehrává, jinak false
 */
bool AudioIsPlaying();

/**
 * @brief Aktualizuje fade-out animaci; volat každý snímek.
 *
 * Pokud běží fade-out, snižuje hlasitost lineárně s časem.
 * Po skončení fade-out zvuk automaticky zastaví a uvolní.
 *
 * @param deltaTime Čas od posledního snímku v sekundách
 */
void AudioUpdate(float deltaTime);

/** @brief Cesta k audio souboru menu hudby. */
extern std::string menuMusicPath;

/** @brief Cesta k audio souboru aktuálně přehrávaného levelu. */
extern std::string currentLevelAudioPath;
