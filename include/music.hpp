#pragma once

#include <SDL2/SDL_mixer.h>
#include <asmodean.h>
#include <string>
#include <vector>

#define FMT_TRACK ASSETS "/bgm%02d.ogg"
#define TEM_PCM ASSETS "/pcm_"
#define EXT_PCM ".ogg"

class MusicPlayer
{

public:
    MusicPlayer();

    void setMusic(const char *);

    void setSound(const char *);

    void playPcm(std::string);

private:
    Mix_Music *mus = NULL;
    SDL_RWops *rw = NULL;
    std::vector<byte> musicBuf;

    void freeMusic();

    void freeOps();

    void freeBuf();

    void playMusic();

    void playSoundFromMem(byte *, size_t, std::string);

    void playFromMem(byte *, size_t, std::string);

    void playFromFile(const char *);
};
