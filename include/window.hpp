#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 576

// #define WINDOW_WIDTH 1280
// #define WINDOW_HEIGHT 720

class WindowManager
{
public:
    WindowManager();

    SDL_Renderer *getRenderer() { return renderer; }

    void toggleFullscreen();

private:
    void setWindowIcon(SDL_Window *);

    SDL_Window *window;

    SDL_Renderer *renderer;
};