

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include "audio.h"
#include <iostream>
#include <algorithm>

std::string menuMusicPath;
std::string currentLevelAudioPath;

static ma_engine   s_engine;
static ma_sound    s_sound;
static bool        s_engineReady   = false;
static bool        s_soundLoaded   = false;
static float       s_volume        = 0.7f;

static bool        s_fading        = false;
static float       s_fadeRemaining = 0.0f;  
static float       s_fadeDuration  = 0.0f;  
static float       s_fadeStartVol  = 0.7f;

bool AudioInit() {
    ma_engine_config cfg = ma_engine_config_init();
    
    cfg.channels    = 2;
    cfg.sampleRate  = 44100;

    if (ma_engine_init(&cfg, &s_engine) != MA_SUCCESS) {
        std::cerr << "[Audio] Nelze inicializovat audio engine!\n";
        return false;
    }
    s_engineReady = true;
    std::cout << "[Audio] Engine inicializovan OK\n";
    return true;
}

void AudioShutdown() {
    if (!s_engineReady) return;
    if (s_soundLoaded) {
        ma_sound_stop(&s_sound);
        ma_sound_uninit(&s_sound);
        s_soundLoaded = false;
    }
    ma_engine_uninit(&s_engine);
    s_engineReady = false;
}

void AudioPlayLoop(const std::string& path, float volume) {
    if (!s_engineReady) return;
    if (path.empty()) { AudioStop(0); return; }

    
    std::string cleanPath = path;
    if (cleanPath.size() >= 2 && cleanPath.front() == '"' && cleanPath.back() == '"')
        cleanPath = cleanPath.substr(1, cleanPath.size() - 2);

    
    if (s_soundLoaded) {
        ma_sound_stop(&s_sound);
        ma_sound_uninit(&s_sound);
        s_soundLoaded = false;
    }
    s_fading = false;

    
    ma_uint32 flags = MA_SOUND_FLAG_STREAM; 
    if (ma_sound_init_from_file(&s_engine, cleanPath.c_str(), flags, nullptr, nullptr, &s_sound) != MA_SUCCESS) {
        std::cerr << "[Audio] Nelze nacist soubor: " << cleanPath << "\n";
        return;
    }

    s_volume = std::clamp(volume, 0.0f, 1.0f);
    ma_sound_set_volume(&s_sound, s_volume);
    ma_sound_set_looping(&s_sound, MA_TRUE);
    ma_sound_start(&s_sound);
    s_soundLoaded = true;
    std::cout << "[Audio] Prehravam: " << cleanPath << " (vol=" << s_volume << ")\n";
}

void AudioStop(int fadeMs) {
    if (!s_engineReady || !s_soundLoaded) return;

    if (fadeMs <= 0) {
        ma_sound_stop(&s_sound);
        ma_sound_uninit(&s_sound);
        s_soundLoaded = false;
        s_fading = false;
    } else {
        
        s_fading        = true;
        s_fadeDuration  = (float)fadeMs / 1000.0f;
        s_fadeRemaining = s_fadeDuration;
        s_fadeStartVol  = s_volume;
    }
}

void AudioSetVolume(float volume) {
    s_volume = std::clamp(volume, 0.0f, 1.0f);
    if (s_engineReady && s_soundLoaded)
        ma_sound_set_volume(&s_sound, s_volume);
}

float AudioGetVolume() {
    return s_volume;
}

bool AudioIsPlaying() {
    if (!s_engineReady || !s_soundLoaded) return false;
    return ma_sound_is_playing(&s_sound) == MA_TRUE;
}

void AudioUpdate(float deltaTime) {
    if (!s_engineReady || !s_soundLoaded) return;

    if (s_fading) {
        s_fadeRemaining -= deltaTime;
        if (s_fadeRemaining <= 0.0f) {
            
            ma_sound_stop(&s_sound);
            ma_sound_uninit(&s_sound);
            s_soundLoaded   = false;
            s_fading        = false;
            s_volume        = s_fadeStartVol; 
        } else {
            float t   = s_fadeRemaining / s_fadeDuration; 
            float vol = s_fadeStartVol * t;
            ma_sound_set_volume(&s_sound, vol);
        }
    }
}
