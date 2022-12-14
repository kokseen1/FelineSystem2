#include <audio.hpp>
#include <utils.hpp>

#include <map>

void Sound::free()
{
    // Free existing sound
    if (mixChunk != NULL)
    {
        Mix_FreeChunk(mixChunk);
        mixChunk = NULL;
    }

    if (rwOps != NULL)
    {
        SDL_RWclose(rwOps);
        rwOps = NULL;
    }
}

void Sound::stop()
{
    name.clear();
    free();
}

void Sound::set(const std::string &n, const int l)
{
    name = n;
    loops = l;

    free();
}

void Sound::play(byte *buf, const size_t sz, const int channel)
{
    Mix_HaltChannel(channel);
    free();

    rwOps = SDL_RWFromConstMem(buf, sz);
    mixChunk = Mix_LoadWAV_RW(rwOps, 0);
    Mix_PlayChannel(channel, mixChunk, loops);
}

// Initialize the audio player
AudioManager::AudioManager(FileManager &fm) : fileManager{fm}
{
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 1024) == -1)
    {
        LOG << Mix_GetError();
    }

    // Set fixed volume for channels
    Mix_VolumeMusic(MUSIC_VOLUME);
    Mix_Volume(-1, SE_VOLUME);
    Mix_Volume(CHANNEL_PCM, PCM_VOLUME);

    // LOG << "AudioManager initialized";
}

void AudioManager::stopSound(const int channel)
{
    Mix_HaltChannel(channel);
    currSounds[channel].stop();
}

// Set the current music file
void AudioManager::setMusic(const std::string name)
{
    // Ensure asset exists in database
    if (!fileManager.inDB(name + MUSIC_EXT))
        return;

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
        fileManager.fetchAssetAndProcess(name + MUSIC_EXT, this, &AudioManager::playMusicFromMem, name);
    }
}

// Play a specified PCM asset
// PCM will only use a fixed channel and never loop
void AudioManager::setPCM(const std::string &name)
{
    Mix_HaltChannel(CHANNEL_PCM);
    currSounds[CHANNEL_PCM].set(name, 0);

    fileManager.fetchAssetAndProcess(name + PCM_EXT, this, &AudioManager::playSoundFromMem, SoundData{CHANNEL_PCM, name});
}

// Play a specified sound effect asset
void AudioManager::setSE(const std::string &name, const int channel, const int loops)
{
    // Ensure channel is within range
    if (channel >= SOUND_CHANNELS)
        return;

    // Ensure asset exists in database
    if (!fileManager.inDB(name + SE_EXT))
        return;

    Mix_HaltChannel(channel);
    currSounds[channel].set(name, loops);

    fileManager.fetchAssetAndProcess(name + SE_EXT, this, &AudioManager::playSoundFromMem, SoundData{channel, name});
}

void AudioManager::playMusic(Mix_Music *mixMusic, const std::string &name)
{
    // Ensure that current music is to be played (in async cases)
    if (name == currMusicName)
        Mix_PlayMusic(mixMusic, -1);
}

// Play a file buffer as music
void AudioManager::playMusicFromMem(byte *buf, size_t sz, const std::string& name)
{
    // Do not play if curr music already changed (async fetch was too slow)
    if (currMusicName != name)
        return;

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
void AudioManager::playSoundFromMem(byte *buf, size_t sz, const SoundData& soundData)
{
    const int channel = soundData.channel;
    const auto &name = soundData.name;

    auto &sound = currSounds[channel];

    // Ensure current sound is to be played (async)
    if (name != sound.getName())
        return;

    sound.play(buf, sz, channel);
}

// Stop any existing music
void AudioManager::stopMusic()
{
    Mix_HaltMusic();
    currMusicName.clear();
}

// Stop all SE
void AudioManager::stopSounds()
{
    for (int i = 0; i < SOUND_CHANNELS; i++)
        stopSound(i);
}

void AudioManager::loadDump(const json &j)
{
    stopSounds();
    if (j.contains(KEY_SE))
    {
        for (int i = 0; i < SOUND_CHANNELS; i++)
        {
            const auto &channel = std::to_string(i);
            if (j[KEY_SE].contains(channel))
                setSE(j[KEY_SE][channel], i, -1);
        }
    }

    stopMusic();
    if (j.contains(KEY_MUSIC) && j[KEY_MUSIC].contains(KEY_NAME))
        setMusic(j[KEY_MUSIC][KEY_NAME]);
}

const json AudioManager::dump()
{
    json j;

    if (!currMusicName.empty())
        j[KEY_MUSIC][KEY_NAME] = currMusicName;

    // Save only looping SE
    for (int i = 0; i < SOUND_CHANNELS; i++)
    {
        auto &name = currSounds[i].getName();
        if (!name.empty() && currSounds[i].getLoops() == -1)
            j[KEY_SE][std::to_string(i)] = name;
    }

    return j;
}
