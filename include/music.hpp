#pragma once

#include <file.hpp>
#include <asmodean.h>

#include <SDL2/SDL_mixer.h>

#include <map>
#include <string>
#include <vector>

#define SOUND_CHANNELS 8
#define CHANNEL_PCM SOUND_CHANNELS - 1

#define MUSIC_EXT ".ogg"
#define PCM_EXT ".ogg"
#define SE_EXT ".ogg"

class MusicManager
{

public:
    MusicManager(FileManager *fileManager);

    void setMusic(const std::string);

    void setPCM(const std::string &);

    void setSE(const std::string &, const int, const int);

    void stopSound(const int);

private:
    FileManager *fileManager = NULL;

    SDL_RWops *soundOps[SOUND_CHANNELS] = {NULL};
    Mix_Chunk *soundChunks[SOUND_CHANNELS] = {NULL};

    Mix_Music *music = NULL;
    SDL_RWops *musicOps = NULL;
    std::vector<byte> musicVec;

    void stopAndFreeMusic();

    void freeOps(SDL_RWops *);

    void freeBuf();

    void playMusicFromFile(const std::string);

    void playMusicFromMem(byte *, size_t, int);

    void playSoundFromMem(byte *, size_t, const std::pair<int, int>);
};
