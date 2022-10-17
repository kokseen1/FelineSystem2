#include <music.hpp>
#include <utils.hpp>

#include <map>

#define EXT_PCM ".ogg"

// Initialize the audio player
MusicManager::MusicManager(FileManager *fm) : fileManager{fm}
{
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 1024) == -1)
    {
        std::cout << Mix_GetError() << std::endl;
    }
    std::cout << "MusicManager initialized" << std::endl;
}

// Play a pcm file via its name
void MusicManager::playPcm(std::string pcm)
{
    std::string fname = pcm + EXT_PCM;
    Utils::lowercase(fname);
    setSound(fname);
}

// Play a file buffer as music
void MusicManager::playMusicFromMem(byte *buf, size_t sz, int userdata)
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
void MusicManager::playSoundFromMem(byte *buf, size_t sz, int userdata)
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
// Set the current sound file
void MusicManager::setSound(std::string fpath)
{
    fileManager->fetchAssetAndProcess(fpath, this, &MusicManager::playSoundFromMem, 0);
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
    if (ops != NULL)
    {
        if (SDL_RWclose(ops) < 0)
        {
            std::cout << "Could not free RWops" << std::endl;
        }
    }
}
