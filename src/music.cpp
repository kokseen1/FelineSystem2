#include <music.hpp>
#include <utils.hpp>

#include <map>

#define EXT_PCM ".ogg"

// Initialize the audio player
MusicPlayer::MusicPlayer()
{
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 1024) == -1)
    {
        printf("%s", Mix_GetError());
    }
    std::cout << "MusicPlayer initialized" << std::endl;
}

// Play a pcm file via its name
void MusicPlayer::playPcm(std::string pcm)
{
    for (auto const &x : pcmMap)
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
    Utils::fetchFileAndProcess(fpath, this, &MusicPlayer::playSoundFromMem);
}

// Set the current music file
void MusicPlayer::setMusic(const std::string fpath)
{
    Utils::fetchFileAndProcess(fpath, this, &MusicPlayer::playMusicFromMem);
}

// Play a file buffer as sound
void MusicPlayer::playSoundFromMem(byte *buf, size_t sz, const std::string &fpath)
{
    if (soundChunk != NULL)
    {
        Mix_FreeChunk(soundChunk);
    }
    freeOps(soundOps);

    soundOps = SDL_RWFromConstMem(buf, sz);
    soundChunk = Mix_LoadWAV_RW(soundOps, 0);
    int channel = Mix_PlayChannel(-1, soundChunk, 0);
}

// Play a file buffer as music
void MusicPlayer::playMusicFromMem(byte *buf, size_t sz, const std::string &fpath)
{
    stopAndFreeMusic();
    freeOps(musicOps);
    musicVec.clear();

    musicVec.insert(musicVec.end(), buf, buf + sz);
    musicOps = SDL_RWFromConstMem(musicVec.data(), sz);
    music = Mix_LoadMUS_RW(musicOps, 0);

    Mix_PlayMusic(music, -1);
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
