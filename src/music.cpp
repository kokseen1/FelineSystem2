#include <stdio.h>
#include <music.hpp>

#include <utils.hpp>

MusicPlayer::MusicPlayer()
{
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 1024) == -1)
    {
        printf("%s", Mix_GetError());
    }
    printf("MusicPlayer initialized\n");
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

void MusicPlayer::setSound(const char *fpath)
{
    printf("Playing pcm %s\n", fpath);
    Utils::processFile(fpath, this, &MusicPlayer::playSoundFromMem);
}

void MusicPlayer::setMusic(const char *fpath)
{
#ifdef __EMSCRIPTEN__
    // Crashes if called too quickly on local
    Utils::processFile(fpath, this, &MusicPlayer::playFromMem);
#else
    playFromFile(fpath);
#endif
}

void MusicPlayer::playSoundFromMem(byte *buf, size_t sz)
{
    auto soundOps = SDL_RWFromMem(buf, sz);
    Mix_Chunk *chunk = Mix_LoadWAV_RW(soundOps, 0);
    int channel = Mix_PlayChannel(-1, chunk, 0);
    // Cannot be called here for local SDL
    // Mix_FreeChunk(chunk);
}

void MusicPlayer::playFromMem(byte *buf, size_t sz)
{
    freeBuf();
    musicBuf = new char[sz];
    SDL_memcpy(musicBuf, buf, sz);

    freeOps();
    rw = SDL_RWFromMem(musicBuf, sz);

    freeMusic();
    // Setting freesrc will free rw upon return
    mus = Mix_LoadMUS_RW(rw, 0);
    // mus = Mix_LoadMUSType_RW(rw, MUS_NONE, 1);

    playMusic();
}

// Play via filename
void MusicPlayer::playFromFile(const char *fpath)
{
    freeMusic();

    mus = Mix_LoadMUS(fpath);
    playMusic();
}

// Free any existing music
void MusicPlayer::freeMusic()
{
    if (mus != NULL)
    {
        Mix_HaltMusic();
        Mix_FreeMusic(mus);
    }
}
// Free any existing SDL_RWops
void MusicPlayer::freeOps()
{
    if (rw != NULL)
    {
        // Will break if next SDL_RWops allocation is at 0xa1;
        if (SDL_RWclose(rw) < 0)
        {
            printf("Error SDL_RWclose\n");
        }
    }
}

// Free any existing buffer
void MusicPlayer::freeBuf()
{
    if (musicBuf != NULL)
    {
        delete[] musicBuf;
    }
}

void MusicPlayer::playMusic()
{
    Mix_PlayMusic(mus, -1);
}
