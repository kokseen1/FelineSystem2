#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

#include <window.hpp>
#include <utils.hpp>

WindowManager::WindowManager()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        throw std::runtime_error("Could not initialize SDL");
    }

    if (TTF_Init() == -1)
    {
        throw std::runtime_error("Could not init TTF");
    }

    // Create SDL window and renderer
    window = SDL_CreateWindow("FelineSystem2",
                              SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              WINDOW_WIDTH,
                              WINDOW_HEIGHT,
                              SDL_WINDOW_SHOWN);
    if (window == NULL)
    {
        throw std::runtime_error("Could not create SDL window");
    }

    setWindowIcon(window);

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == NULL)
    {
        throw std::runtime_error("Could not create SDL renderer");
    }
}

// Set the SDL window icon using an array of pixels
void WindowManager::setWindowIcon(SDL_Window *window)
{
#include <logo.h>
    SDL_Surface *surface = SDL_CreateRGBSurfaceFrom(logo, 64, 64, 32, 64 * 4, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
    if (surface == NULL)
    {
        LOG << "Could not create surface from logo";
        return;
    }

    SDL_SetWindowIcon(window, surface);
    SDL_FreeSurface(surface);
}

void WindowManager::toggleFullscreen()
{
#ifdef __EMSCRIPTEN__
    // Much better for mobile
    EmscriptenFullscreenChangeEvent fsce;
    EMSCRIPTEN_RESULT ret = emscripten_get_fullscreen_status(&fsce);
    if (!fsce.isFullscreen)
    {
        ret = emscripten_request_fullscreen("#canvas", 1);
    }
    else
    {
        ret = emscripten_exit_fullscreen();
        ret = emscripten_get_fullscreen_status(&fsce);
    }
#else
    // SDL fullscreen
    static bool isFullscreen = false;
    switch (isFullscreen)
    {
    case false:
        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
        break;
    case true:
        SDL_SetWindowFullscreen(window, 0);
        break;
    }
    isFullscreen = !isFullscreen;
#endif
}