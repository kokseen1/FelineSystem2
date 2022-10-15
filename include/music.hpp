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

class MusicPlayer
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
    MusicPlayer(FileManager *fileManager);

    void setMusic(const std::string);

    void setSound(const std::string);

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

    // Play a file buffer as music
    template <typename T>
    void playMusicFromMem(byte *buf, size_t sz, T userdata)
    {
        stopAndFreeMusic();
        freeOps(musicOps);
        musicVec.clear();

        musicVec.insert(musicVec.end(), buf, buf + sz);
        musicOps = SDL_RWFromConstMem(musicVec.data(), sz);
        music = Mix_LoadMUS_RW(musicOps, 0);

        Mix_PlayMusic(music, -1);
    }

    // Play a file buffer as sound
    template <typename T>
    void playSoundFromMem(byte *buf, size_t sz, T userdata)
    {
        if (soundChunk != NULL)
        {
            Mix_HaltChannel(CHANNEL_SOUND);
            Mix_FreeChunk(soundChunk);
        }
        freeOps(soundOps);

        soundOps = SDL_RWFromConstMem(buf, sz);
        soundChunk = Mix_LoadWAV_RW(soundOps, 0);
        Mix_PlayChannel(CHANNEL_SOUND, soundChunk, 0);
    }
};
