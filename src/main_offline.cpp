#include <stdio.h>
#include <SDL2/SDL.h>

#include <fstream>
#include <vector>

#include <music.hpp>
#include <image.hpp>

int main(int argc, char **argv)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
        return 1;
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 1024) == -1)
    {
        printf("%s", Mix_GetError());
    }

    // Only for offline
    // Request a window to be created for our platform
    SDL_Window *window = SDL_CreateWindow("FelineSystem2",
                                          SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                          1024,
                                          576,
                                          SDL_WINDOW_SHOWN);
    SDL_Surface *screen = SDL_GetWindowSurface(window);

    MusicPlayer *musicPlayer = new MusicPlayer();
    std::ifstream infile("assets/bgm01.ogg", std::ios_base::binary);
    std::vector<char> buffer((std::istreambuf_iterator<char>(infile)),
                             (std::istreambuf_iterator<char>()));
    musicPlayer->playFromMem(buffer.data(), buffer.size());

    std::ifstream hgfile("assets/bg09.hg3", std::ios_base::binary);
    // std::ifstream hgfile("assets/activate.hg3", std::ios_base::binary);
    std::vector<char> hgbuf((std::istreambuf_iterator<char>(hgfile)),
                            (std::istreambuf_iterator<char>()));
    HGHeader *hgHeader = (HGHeader *)hgbuf.data();

    for (auto frame : HGDecoder::getFrames(&hgHeader->FrameHeaderStart))
    {
        SDL_Surface *flipped = HGDecoder::getSurfaceFromFrame(frame);

        // Platform specific displaying
        SDL_BlitSurface(flipped, NULL, screen, NULL);
        SDL_UpdateWindowSurface(window);

        SDL_FreeSurface(flipped);
    }

    SDL_Delay(10000);

    return 0;
}
