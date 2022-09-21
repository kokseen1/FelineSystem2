#include <music.hpp>
#include <utils.hpp>

// Initialize the audio player
MusicPlayer::MusicPlayer()
{
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 1024) == -1)
    {
        printf("%s", Mix_GetError());
    }
    std::cout << "MusicPlayer initialized" << std::endl;
}

void MusicPlayer::playPcm(std::string pcm)
{
    std::string fpath;
    if (pcm.rfind("YUM", 0) == 0)
    {
        fpath = TEM_PCM "a/" + pcm;
    }
    else if (pcm.rfind("AMA", 0) == 0)
    {
        fpath = TEM_PCM "b/" + pcm;
    }
    else
    {
        return;
    }

    fpath += EXT_PCM;
    setSound(fpath.c_str());
}

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
    SDL_RWops *soundOps = SDL_RWFromConstMem(buf, sz);
    Mix_Chunk *chunk = Mix_LoadWAV_RW(soundOps, 0);
    int channel = Mix_PlayChannel(-1, chunk, 0);
    // Cannot be called here for local SDL
    // Mix_FreeChunk(chunk);
}

// Play a file buffer as music
void MusicPlayer::playMusicFromMem(byte *buf, size_t sz, const std::string &fpath)
{
    stopAndFreeMusic();
    freeMusicOps();
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
void MusicPlayer::freeMusicOps()
{
    if (musicOps != NULL)
    {
        if (SDL_RWclose(musicOps) < 0)
        {
            printf("Error SDL_RWclose\n");
        }
    }
}
