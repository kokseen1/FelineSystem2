#pragma once

#include <SDL2/SDL.h>
#include <asmodean.h>
#include <string>
#include <map>

#define FMT_SCENE ASSETS "/image/bg%02d.hg3"
#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 576

class SceneManager
{

public:
    SceneManager();

    void setScene(const char *);

    void displayFrame(byte *, size_t, std::string);

    SDL_Window *window = NULL;

    SDL_Renderer *renderer = NULL;

private:
    std::map<std::string, SDL_Texture *> textureCache;

    static void onLoad(void *, void *, int);
    static void onError(void *);
};
