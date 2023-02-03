#pragma once

#include <file.hpp>
#include <asmodean.h>

#include <SDL2/SDL_mixer.h>

#include <unordered_map>
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

typedef std::unordered_map<std::string, Mix_Music *> MusicCache;

class Sound
{
private:
    std::string name;
    int loops;

    SDL_RWops *rwOps = NULL;
    Mix_Chunk *mixChunk = NULL;

    void free();

public:
    std::string &getName() { return name; };

    auto getLoops() { return loops; };

    void stop();

    void set(const std::string &, const int);

    void play(byte *, const size_t, const int);
};

typedef struct
{
    int channel;
    std::string name;
} SoundData;

class AudioManager
{

public:
    AudioManager(FileManager &fileManager);

    void setMusic(const std::string);

    void setPCM(const std::string &);

    void setSE(const std::string &, const int, const int);

    void fadeOutSound(const int, const int);

    void stopSound(const int);

    void loadDump(const json &);

    const json dump();

private:
    FileManager &fileManager ;
    MusicCache musicCache;

    std::array<Sound, SOUND_CHANNELS> currSounds;

    std::vector<std::vector<byte>> musicBufVec;

    std::string currMusicName;

    void stopSounds();

    void stopMusic();

    void freeBuf();

    void playMusic(Mix_Music *, const std::string &);

    void playMusicFromMem(byte *, size_t, const std::string&);

    void playSoundFromMem(byte *, size_t, const SoundData&);
};
