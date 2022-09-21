#include <stdio.h>
#include <scene.hpp>
#include <hgdecoder.hpp>

#include <utils.hpp>

SceneManager::SceneManager()
{
    window = SDL_CreateWindow("FelineSystem2",
                              SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              WINDOW_WIDTH,
                              WINDOW_HEIGHT,
                              SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    printf("SceneManager initialized\n");
}

// Decode a HG buffer and display the first frame
void SceneManager::displayFrame(byte *buf, size_t sz, std::string fpath)
{
    HGHeader *hgHeader = reinterpret_cast<HGHeader *>(buf);
    FrameHeader *frameHeader = reinterpret_cast<FrameHeader *>(hgHeader + 1);

    std::vector<HGDecoder::Frame> frames = HGDecoder::getFrames(frameHeader);

    for (auto frame : frames)
    {
        SDL_Surface *surface = HGDecoder::getSurfaceFromFrame(frame);

        SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);

        SDL_Rect DestR = {0, 0, static_cast<int>(frame.Stdinfo->Width), static_cast<int>(frame.Stdinfo->Height)};

        SDL_RenderCopy(renderer, texture, NULL, &DestR);
        SDL_RenderPresent(renderer);

        // Cache texture
        textureCache.insert({fpath, texture});
        // SDL_DestroyTexture(texture);

        // CPU rendering
        // SDL_Surface *screen = SDL_GetWindowSurface(window);
        // SDL_BlitSurface(surface, NULL, screen, NULL);
        // SDL_UpdateWindowSurface(window);

        SDL_FreeSurface(surface);

        // Just handle the first frame for now
        break;
    }
}

void SceneManager::setScene(const char *fpath)
{
    if (textureCache.find(fpath) != textureCache.end())
    {
        SDL_RenderCopy(renderer, textureCache[fpath], NULL, NULL);
        SDL_RenderPresent(renderer);
        return;
    }
    Utils::processFile(fpath, this, &SceneManager::displayFrame);
}
