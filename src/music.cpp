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
    for (auto const &x : pcmPathMap)
    {
        if (pcm.rfind(x.first) == 0)
        {
            std::string fpath = ASSETS + x.second + pcm + EXT_PCM;
            setSound(fpath);
            return;
        }
    }
}

// Set the current sound file
void MusicManager::setSound(std::string fpath)
{
    fileManager->fetchFileAndProcess(fpath, this, &MusicManager::playSoundFromMem, NULL);
}

// Set the current music file
void MusicManager::setMusic(const std::string name)
{
    auto fpath = ASSETS MUSIC_PATH + name + MUSIC_EXT;
    fileManager->fetchFileAndProcess(fpath, this, &MusicManager::playMusicFromMem, NULL);
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
