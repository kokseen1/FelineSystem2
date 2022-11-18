#pragma once

#include <file.hpp>
#include <asmodean.h>

#include <SDL2/SDL_mixer.h>

#include <map>
#include <string>
#include <vector>

#define SOUND_CHANNELS 8
#define CHANNEL_PCM SOUND_CHANNELS - 1

#define MUSIC_VOLUME 60
#define SE_VOLUME 50
#define PCM_VOLUME SDL_MIX_MAXVOLUME

#define MUSIC_EXT ".ogg"
#define PCM_EXT ".ogg"
#define SE_EXT ".ogg"

#define KEY_MUSIC "music"
#define KEY_SE "se"
#define KEY_LOOPS "loops"
#define KEY_NAME "name"

class MusicManager
{

public:
    MusicManager(FileManager *fileManager);

    void setMusic(const std::string);

    void setPCM(const std::string &);

    void setSE(const std::string &, const int, const int);

    void stopSound(const int);

    void loadDump(const json &);

    const json dump();

private:
    FileManager *fileManager = NULL;

    std::array<SDL_RWops *, SOUND_CHANNELS> soundOps;
    std::array<Mix_Chunk *, SOUND_CHANNELS> soundChunks;

    Mix_Music *music = NULL;
    SDL_RWops *musicOps = NULL;
    std::vector<byte> musicVec;

    std::string currMusic;

    void stopAndFreeMusic();

    void freeOps(SDL_RWops *);

    void freeBuf();

    void playMusicFromMem(byte *, size_t, const std::string);

    void playSoundFromMem(byte *, size_t, const std::pair<int, int>);
};
