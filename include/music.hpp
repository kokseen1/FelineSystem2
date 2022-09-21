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
    Mix_Music *music = NULL;
    SDL_RWops *musicOps = NULL;
    std::vector<byte> musicVec;

    void stopAndFreeMusic();

    void freeMusicOps();

    void freeBuf();

    void playSoundFromMem(byte *, size_t, const std::string &);

    void playMusicFromMem(byte *, size_t, const std::string &);

    void playMusicFromFile(const std::string);
};
