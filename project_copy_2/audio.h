

#pragma once
#include <string>

bool AudioInit();

void AudioShutdown();

void AudioPlayLoop(const std::string& path, float volume = 0.7f);

void AudioStop(int fadeMs = 400);

void AudioSetVolume(float volume);

float AudioGetVolume();

bool AudioIsPlaying();

void AudioUpdate(float deltaTime);

extern std::string menuMusicPath;    
extern std::string currentLevelAudioPath; 
