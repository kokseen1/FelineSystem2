#pragma once

#include <SDL2/SDL.h>
#include <asmodean.h>

#define FMT_SCENE "assets/bg%02d.hg3"
#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 576

class SceneManager
{

public:
    SceneManager();

    void setScene(char *, int);

    void displayFrame(byte *);

    SDL_Window *window = NULL;

    SDL_Renderer *renderer = NULL;

private:
    static void onLoad(void *, void *, int);
    static void onError(void *);
};
