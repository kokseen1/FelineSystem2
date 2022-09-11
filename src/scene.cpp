#include <stdio.h>
#include <scene.hpp>
#include <hgdecoder.hpp>

#include <fstream>
#include <vector>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

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

void SceneManager::displayFrame(void *buf)
{
    HGHeader *hgHeader = (HGHeader *)buf;
    FrameHeader *frameHeader = &hgHeader->FrameHeaderStart;

    std::vector<Frame> frames = HGDecoder::getFrames(frameHeader);

    for (auto frame : frames)
    {
        SDL_Surface *surface = HGDecoder::getSurfaceFromFrame(frame);

#ifdef __EMSCRIPTEN__
        SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
        // SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
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

void SceneManager::onLoad(void *arg, void *buf, int sz)
{
    // buf will be freed after returning from this callback
    SceneManager *sceneManager = (SceneManager *)arg;
    sceneManager->displayFrame(buf);
}

void SceneManager::onError(void *arg)
{
    printf("SceneManager onError\n");
}

void SceneManager::setScene(char *fpath, int local)
{
    if (local)
    {
        // Does not work well with emscripten
        // std::ifstream hgfile(fpath, std::ios_base::binary);
        // std::vector<char> buffer((std::istreambuf_iterator<char>(hgfile)),
        //                          (std::istreambuf_iterator<char>()));
        // displayFrame(buffer.data());

        // Read file and get size
        FILE *fp = fopen(fpath, "rb");
        fseek(fp, 0, SEEK_END);
        int sz = ftell(fp);
        rewind(fp);

        // Read file into buffer
        char *buf = (char *)malloc(sz * sizeof(char));
        fread(buf, sizeof(char), sz, fp);
        displayFrame(buf);

        free(buf);
    }
    else
    {
        emscripten_async_wget_data(fpath, this, onLoad, onError);
    }
}
