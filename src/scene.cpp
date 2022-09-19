#include <stdio.h>
#include <scene.hpp>
#include <hgdecoder.hpp>

#include <utils.hpp>

SceneManager::SceneManager()
{

#ifdef __EMSCRIPTEN__
    SDL_CreateWindowAndRenderer(WINDOW_WIDTH, WINDOW_HEIGHT, 0, &window, &renderer);
#else
    window = SDL_CreateWindow("FelineSystem2",
                              SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              WINDOW_WIDTH,
                              WINDOW_HEIGHT,
                              SDL_WINDOW_SHOWN);
#endif
    printf("SceneManager initialized\n");
}

// Decode a HG buffer and display the first frame
void SceneManager::displayFrame(byte *buf, size_t sz)
{
    HGHeader *hgHeader = reinterpret_cast<HGHeader *>(buf);
    FrameHeader *frameHeader = reinterpret_cast<FrameHeader *>(hgHeader + 1);

    std::vector<HGDecoder::Frame> frames = HGDecoder::getFrames(frameHeader);

    for (auto frame : frames)
    {
        SDL_Surface *surface = HGDecoder::getSurfaceFromFrame(frame);

#ifdef __EMSCRIPTEN__
        SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);

        SDL_RenderClear(renderer);

        SDL_Rect DestR;
        DestR.x = 0;
        DestR.y = 0;
        DestR.w = frame.Stdinfo->Width;
        DestR.h = frame.Stdinfo->Height;

        SDL_RenderCopy(renderer, texture, NULL, &DestR);
        SDL_RenderPresent(renderer);

        SDL_DestroyTexture(texture);
#else
        SDL_Surface *screen = SDL_GetWindowSurface(window);
        SDL_BlitSurface(surface, NULL, screen, NULL);
        SDL_UpdateWindowSurface(window);
#endif
        SDL_FreeSurface(surface);

        // Just handle the first frame for now
        break;
    }
}

void SceneManager::setScene(char *fpath)
{
    Utils::processFile(fpath, this, &SceneManager::displayFrame);
}
