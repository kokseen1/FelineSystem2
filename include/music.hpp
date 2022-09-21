#pragma once

#include <SDL2/SDL_mixer.h>
#include <asmodean.h>
#include <string>
#include <vector>

#define TEM_PCM ASSETS "/pcm_"
#define EXT_PCM ".ogg"

class MusicPlayer
{

public:
    MusicPlayer();

    void setMusic(const std::string);

    void setSound(const std::string);

    void playPcm(std::string);

private:
    Mix_Music *mus = NULL;
    SDL_RWops *rw = NULL;
    std::vector<byte> musicBuf;

    void freeMusic();

    void freeOps();

    void freeBuf();

    void playMusic();

    void playSoundFromMem(byte *, size_t, const std::string &);

    void playFromMem(byte *, size_t, const std::string &);

    void playFromFile(const char *);
};
