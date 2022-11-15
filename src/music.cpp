#include <music.hpp>
#include <utils.hpp>

#include <map>

// Initialize the audio player
MusicManager::MusicManager(FileManager *fm) : fileManager{fm}
{
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 1024) == -1)
    {
        std::cout << Mix_GetError() << std::endl;
    }

    // Set fixed volume for channels
    Mix_VolumeMusic(MUSIC_VOLUME);
    Mix_Volume(-1, SE_VOLUME);
    Mix_Volume(CHANNEL_PCM, PCM_VOLUME);

    LOG << "MusicManager initialized";
}

// Play a specified PCM asset
// PCM will only use a fixed channel and never loop
void MusicManager::setPCM(const std::string &asset)
{
    fileManager->fetchAssetAndProcess(asset + PCM_EXT, this, &MusicManager::playSoundFromMem, std::make_pair(CHANNEL_PCM, 0));
}

// Play a specified sound effect asset
void MusicManager::setSE(const std::string &asset, const int channel, const int loops)
{
    fileManager->fetchAssetAndProcess(asset + SE_EXT, this, &MusicManager::playSoundFromMem, std::make_pair(channel, loops));
}

// Play a file buffer as music
void MusicManager::playMusicFromMem(byte *buf, size_t sz, int userdata)
{
    stopAndFreeMusic();
    if (musicOps != NULL)
        freeOps(musicOps);
    musicVec.clear();

    musicVec.insert(musicVec.end(), buf, buf + sz);
    musicOps = SDL_RWFromConstMem(musicVec.data(), sz);
    music = Mix_LoadMUS_RW(musicOps, 0);

    Mix_PlayMusic(music, -1);
}

// Play a file buffer as sound
void MusicManager::playSoundFromMem(byte *buf, size_t sz, const std::pair<int, int> channelLoops)
{
    const int &channel = channelLoops.first;
    const int &loops = channelLoops.second;

    if (channel >= SOUND_CHANNELS)
    {
        LOG << "CHANNEL OUT OF BOUNDS!";
        return;
    }

    stopSound(channel);

    soundOps[channel] = SDL_RWFromConstMem(buf, sz);
    soundChunks[channel] = Mix_LoadWAV_RW(soundOps[channel], 0);
    Mix_PlayChannel(channel, soundChunks[channel], loops);
}

// Stops a sound and frees the channel
void MusicManager::stopSound(const int channel)
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
void MusicManager::setMusic(const std::string name)
{
    auto fpath = name + MUSIC_EXT;
    fileManager->fetchAssetAndProcess(fpath, this, &MusicManager::playMusicFromMem, 0);
}

// Play a local file via filename
void MusicManager::playMusicFromFile(const std::string fpath)
{
    stopAndFreeMusic();
    music = Mix_LoadMUS(fpath.c_str());
    if (music == NULL)
    {
        std::cout << "Could not play " << fpath << std::endl;
        return;
    }

    Mix_PlayMusic(music, -1);
}

// Stop and free any existing music
void MusicManager::stopAndFreeMusic()
{
    if (music != NULL)
    {
        Mix_HaltMusic();
        Mix_FreeMusic(music);
    }
}

// Free any existing music SDL_RWops
void MusicManager::freeOps(SDL_RWops *ops)
{
    if (SDL_RWclose(ops) < 0)
    {
        LOG << "Could not free RWops";
    }
}
