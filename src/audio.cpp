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

inline void AudioManager::playMusic(Mix_Music *mixMusic, const std::string &name)
{
    // Ensure that current music is to be played (in async cases)
    if (name == currMusicName)
        Mix_PlayMusic(mixMusic, -1);
}

// Play a file buffer as music
void AudioManager::playMusicFromMem(byte *buf, size_t sz, const std::string name)
{
    // Store raw buffer as a byte vector in a global vector and retrieve reference to it
    musicBufVec.push_back({buf, buf + sz});
    const auto &musicBuf = musicBufVec.back();

    // musicOps is not freed as all music will remain cached throughout the program lifetime
    auto musicOps = SDL_RWFromConstMem(musicBuf.data(), musicBuf.size());

    // Create music object and play
    auto mixMusic = Mix_LoadMUS_RW(musicOps, 1);
    playMusic(mixMusic, name);

    // Cache music
    musicCache[name] = mixMusic;
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
    currMusicName = name;

    auto mixMusic = musicCache[name];
    if (mixMusic != NULL)
    {
        // Play directly from cache
        playMusic(mixMusic, name);
    }
    else
    {
        // Fetch and store in cache
        auto fpath = name + MUSIC_EXT;
        fileManager->fetchAssetAndProcess(fpath, this, &AudioManager::playMusicFromMem, name);
    }
}

// Stop any existing music
void AudioManager::stopMusic()
{
    Mix_HaltMusic();
    currMusicName.clear();
}

// Free any existing music SDL_RWops
inline void AudioManager::freeOps(SDL_RWops *ops)
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

    stopMusic();

    if (j.contains(KEY_MUSIC) && j[KEY_MUSIC].contains(KEY_NAME))
    {
        setMusic(j[KEY_MUSIC][KEY_NAME]);
    }
}

const json AudioManager::dump()
{
    json j;

    // TODO: dump (looping) SE

    if (!currMusicName.empty())
    {
        j[KEY_MUSIC][KEY_NAME] = currMusicName;
    }

    return j;
}
