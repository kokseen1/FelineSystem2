#pragma once

#include <file.hpp>
#include <asmodean.h>

#include <SDL2/SDL_mixer.h>

#include <map>
#include <string>
#include <vector>

#define CHANNEL_SOUND 0
#define MUSIC_EXT ".ogg"

class MusicManager
{

public:
    MusicManager(FileManager *fileManager);

    void setMusic(const std::string);

    void setSound(std::string);

    void playPcm(std::string);

private:
    FileManager *fileManager = NULL;

    Mix_Music *music = NULL;
    Mix_Chunk *soundChunk = NULL;
    SDL_RWops *musicOps = NULL;
    SDL_RWops *soundOps = NULL;
    std::vector<byte> musicVec;

    void stopAndFreeMusic();

    void freeOps(SDL_RWops *);

    void freeBuf();

    void playMusicFromFile(const std::string);

    void playMusicFromMem(byte *, size_t, int);

    void playSoundFromMem(byte *, size_t, int);
};
