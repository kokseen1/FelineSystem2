#include <audio.hpp>
#include <utils.hpp>

#include <map>

// Initialize the audio player
AudioManager::AudioManager(FileManager *fm) : fileManager{fm}
{
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 1024) == -1)
    {
        LOG << Mix_GetError();
    }

    // Set fixed volume for channels
    Mix_VolumeMusic(MUSIC_VOLUME);
    Mix_Volume(-1, SE_VOLUME);
    Mix_Volume(CHANNEL_PCM, PCM_VOLUME);

    LOG << "AudioManager initialized";
}

// Play a specified PCM asset
// PCM will only use a fixed channel and never loop
void AudioManager::setPCM(const std::string &asset)
{
    fileManager->fetchAssetAndProcess(asset + PCM_EXT, this, &AudioManager::playSoundFromMem, std::make_pair(CHANNEL_PCM, 0));
}

// Play a specified sound effect asset
void AudioManager::setSE(const std::string &asset, const int channel, const int loops)
{
    fileManager->fetchAssetAndProcess(asset + SE_EXT, this, &AudioManager::playSoundFromMem, std::make_pair(channel, loops));
}

// Play a file buffer as music
void AudioManager::playMusicFromMem(byte *buf, size_t sz, const std::string name)
{
    stopAndFreeMusic();
    if (musicOps != NULL)
        freeOps(musicOps);
    musicVec.clear();

    musicVec.insert(musicVec.end(), buf, buf + sz);
    musicOps = SDL_RWFromConstMem(musicVec.data(), sz);
    music = Mix_LoadMUS_RW(musicOps, 0);

    Mix_PlayMusic(music, -1);

    currMusic = name;
}

// Play a file buffer as sound
void AudioManager::playSoundFromMem(byte *buf, size_t sz, const std::pair<int, int> channelLoops)
{
    const int &channel = channelLoops.first;
    const int &loops = channelLoops.second;

    if (channel >= SOUND_CHANNELS)
    {
        LOG << "Channel out of bounds!";
        return;
    }

    stopSound(channel);

    soundOps[channel] = SDL_RWFromConstMem(buf, sz);
    soundChunks[channel] = Mix_LoadWAV_RW(soundOps[channel], 0);
    Mix_PlayChannel(channel, soundChunks[channel], loops);
}

// Stops a sound and frees the channel
void AudioManager::stopSound(const int channel)
{
    if (soundChunks[channel] != NULL)
    {
        Mix_HaltChannel(channel);
        Mix_FreeChunk(soundChunks[channel]);
        soundChunks[channel] = NULL;
    }

    if (soundOps[channel] != NULL)
    {
        freeOps(soundOps[channel]);
        soundOps[channel] = NULL;
    }
}

// Set the current music file
void AudioManager::setMusic(const std::string name)
{
    auto fpath = name + MUSIC_EXT;
    fileManager->fetchAssetAndProcess(fpath, this, &AudioManager::playMusicFromMem, name);
}

// Stop and free any existing music
void AudioManager::stopAndFreeMusic()
{
    if (music != NULL)
    {
        Mix_HaltMusic();
        Mix_FreeMusic(music);
        music = NULL;
    }
    currMusic.clear();
}

// Free any existing music SDL_RWops
void AudioManager::freeOps(SDL_RWops *ops)
{
    if (SDL_RWclose(ops) < 0)
    {
        LOG << "Could not free RWops";
    }
}

void AudioManager::loadDump(const json &j)
{
    // Stop all SE
    for (int i = 0; i < soundChunks.size(); i++)
        stopSound(i);

    stopAndFreeMusic();

    if (j.contains(KEY_MUSIC) && j[KEY_MUSIC].contains(KEY_NAME))
    {
        setMusic(j[KEY_MUSIC][KEY_NAME]);
    }
}

const json AudioManager::dump()
{
    json j;

    // TODO: dump (looping) SE

    if (!currMusic.empty())
    {
        j[KEY_MUSIC][KEY_NAME] = currMusic;
    }

    return j;
}
