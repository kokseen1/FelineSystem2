#pragma once

#include <SDL2/SDL_mixer.h>
#include <asmodean.h>
#include <string>

#define FMT_TRACK "assets/bgm%02d.ogg"
#define TEM_PCM "assets/pcm_"
#define EXT_PCM ".ogg"

class MusicPlayer
{

public:
    MusicPlayer();

    void setMusic(const char *);
    void setSound(const char *);

    void playSoundFromMem(byte *, int);

    void playFromMem(byte *, int);

    void playFromFile(const char *);

    void playPcm(std::string);

private:
    Mix_Music *mus = NULL;
    SDL_RWops *rw = NULL;
    char *musicBuf = NULL;

    void freeMusic();

    void freeOps();

    void freeBuf();

    void playMusic();

    static void onLoadSound(void *, void *, int);
    static void onLoad(void *, void *, int);
    static void onError(void *);
};
