#pragma once

#include <SDL2/SDL_mixer.h>

#define FMT_TRACK "assets/bgm%02d.ogg"

class MusicPlayer
{

public:
    MusicPlayer();

    void setMusic(char *);

    void playFromMem(void *, int);

    void playFromFile(char *);

private:
    Mix_Music *mus = NULL;
    SDL_RWops *rw = NULL;
    char *musicBuf = NULL;

    void freeMusic();

    void freeOps();

    void freeBuf();

    void playMusic();

    static void onLoad(void *, void *, int);
    static void onError(void *);
};
