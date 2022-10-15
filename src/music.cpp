#include <music.hpp>
#include <utils.hpp>

#include <map>

#define EXT_PCM ".ogg"

// Initialize the audio player
MusicPlayer::MusicPlayer(FileManager *fm) : fileManager{fm}
{
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 1024) == -1)
    {
        std::cout << Mix_GetError() << std::endl;
    }
    std::cout << "MusicPlayer initialized" << std::endl;
}

// Play a pcm file via its name
void MusicPlayer::playPcm(std::string pcm)
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
void MusicPlayer::setSound(const std::string fpath)
{
    fileManager->fetchFileAndProcess(fpath, this, &MusicPlayer::playSoundFromMem, NULL);
}

// Set the current music file
void MusicPlayer::setMusic(const std::string name)
{
    auto fpath = ASSETS MUSIC_PATH + name + MUSIC_EXT;
    fileManager->fetchFileAndProcess(fpath, this, &MusicPlayer::playMusicFromMem, NULL);
}

// Play a local file via filename
void MusicPlayer::playMusicFromFile(const std::string fpath)
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
void MusicPlayer::stopAndFreeMusic()
{
    if (music != NULL)
    {
        Mix_HaltMusic();
        Mix_FreeMusic(music);
    }
}

// Free any existing music SDL_RWops
void MusicPlayer::freeOps(SDL_RWops *ops)
{
    if (ops != NULL)
    {
        if (SDL_RWclose(ops) < 0)
        {
            std::cout << "Could not free RWops" << std::endl;
        }
    }
}
