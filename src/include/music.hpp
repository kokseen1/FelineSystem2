#pragma once

#include <SDL2/SDL_mixer.h>

#define FMT_TRACK "assets/bgm%02d.ogg"

class MusicPlayer
{

public:
    MusicPlayer();

    void playFromMem(void *, int);

    void playFromFile(char *);

    void init();

private:
    Mix_Music *mus = NULL;
    SDL_RWops *rw = NULL;
    char *musicBuf = NULL;

    void freeMusic();

    void freeOps();

    void freeBuf();

    void playMusic();
};
