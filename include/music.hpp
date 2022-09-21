#pragma once

#include <SDL2/SDL_mixer.h>
#include <asmodean.h>

#include <map>
#include <string>
#include <vector>

class MusicPlayer
{
    const std::map<std::string, std::string> pcmMap = {
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
    MusicPlayer();

    void setMusic(const std::string);

    void setSound(const std::string);

    void playPcm(std::string);

private:
    Mix_Music *music = NULL;
    Mix_Chunk *soundChunk = NULL;
    SDL_RWops *musicOps = NULL;
    SDL_RWops *soundOps = NULL;
    std::vector<byte> musicVec;

    void stopAndFreeMusic();

    void freeOps(SDL_RWops *);

    void freeBuf();

    void playSoundFromMem(byte *, size_t, const std::string &);

    void playMusicFromMem(byte *, size_t, const std::string &);

    void playMusicFromFile(const std::string);
};
