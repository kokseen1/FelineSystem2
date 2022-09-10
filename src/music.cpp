#include <music.hpp>

MusicPlayer::MusicPlayer()
{
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 1024) == -1)
    {
        printf("%s", Mix_GetError());
    }
    printf("MusicPlayer initialized\n");
}

void MusicPlayer::playFromMem(void *buf, int sz)
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

    // printf("rw: %p\n", rw);
    // printf("mus: %p\n", mus);
    // printf("musicBuf: %p\n", musicBuf);
}

// Play via filename
void MusicPlayer::playFromFile(char *fpath)
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
        printf("Mix_FreeMusic %p\n", mus);
        Mix_HaltMusic();
        Mix_FreeMusic(mus);
    }
}
// Free any existing SDL_RWops
void MusicPlayer::freeOps()
{
    if (rw != NULL)
    {
        printf("SDL_RWclose %p\n", rw);
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
        printf("freeBuf %p\n", musicBuf);
        delete[] musicBuf;
    }
}

void MusicPlayer::playMusic()
{
    Mix_PlayMusic(mus, -1);
}

void MusicPlayer::init()
{
    mus = NULL;
    rw = NULL;
    musicBuf = NULL;
}