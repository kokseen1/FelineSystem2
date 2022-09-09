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

    // Play via filename
    void playFromFile(char *);

    void init();

private:
    Mix_Music *mus = NULL;
    SDL_RWops *rw = NULL;
    char *musicBuf = NULL;

    // Free any existing music
    void freeMusic();

    // Free any existing SDL_RWops
    void freeOps();

    // Free any existing buffer
    void freeBuf();

    void playMusic();
};
