#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 576

// #define WINDOW_WIDTH 1280
// #define WINDOW_HEIGHT 720

#define MWND "sys_mwnd_43"
#define MWND_DECO "sys_mwnd_42"
#define MWND_ALPHA 130

// Determines length before wrapping text
#define TEXTBOX_WIDTH 650

// Centered position of message window
#define MWND_XSHIFT (WINDOW_WIDTH - 887) / 2
#define MWND_YSHIFT WINDOW_HEIGHT - 154

// Text is relative to center of window based on width
#define TEXT_XPOS (WINDOW_WIDTH - TEXTBOX_WIDTH) / 2 + 50
#define TEXT_YPOS MWND_YSHIFT + 30

// Speaker name is relative to text position
#define SPEAKER_XPOS TEXT_XPOS
#define SPEAKER_YPOS MWND_YSHIFT + 10

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