#pragma once

#include <SDL2/SDL_mixer.h>
#include <asmodean.h>

#include <map>
#include <string>
#include <vector>

#include <file.hpp>

#define CHANNEL_SOUND 0
#define MUSIC_PATH "bgm/"
#define MUSIC_EXT ".ogg"

class MusicManager
{
    const std::map<std::string, std::string> pcmPathMap = {
        {"YUM", "pcm_a/"},
        {"AMA", "pcm_b/"},
        {"MIC", "pcm_c/"},
        {"MAK", "pcm_d/"},
        {"SAC", "pcm_e/"},
        {"SAC", "pcm_e/"},
        {"CHI", "pcm_f/"},
        {"JB", "pcm_g/"},
        {"KAZ", "pcm_h/"},
        {"MOB", "pcm_i/"},
    };

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

    void playMusicFromMem(byte *, size_t , int );

    void playSoundFromMem(byte *, size_t , int );
    
};
