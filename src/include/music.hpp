#pragma once

#include <stdio.h>
#include <SDL2/SDL_mixer.h>

#define TRACK "assets/bgm%02d.ogg"

class MusicPlayer
{

public:
    MusicPlayer()
    {
        printf("Init MusicPlayer\n");
        // init();
    }

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
