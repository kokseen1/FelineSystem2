#include <music.hpp>
#include <image.hpp>
#include <scene.hpp>
#include <file.hpp>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif
#include <stdio.h>
#include <SDL2/SDL.h>

#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>

void main_loop()
{
    // Initialize manager classes
    static FileManager fileManager;
    static MusicManager musicManager(&fileManager);
    static ImageManager imageManager(&fileManager);
    static SceneManager sceneManager(&musicManager, &imageManager, &fileManager);

    static SDL_Event event;

    // Render the canvas
    imageManager.render();

    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_USEREVENT:
            break;

        case SDL_MOUSEWHEEL:
            sceneManager.parseNext();
            break;

        case SDL_MOUSEBUTTONDOWN:
            sceneManager.parseNext();
            break;

        case SDL_KEYDOWN:
            switch (event.key.keysym.sym)
            {
            case SDLK_LCTRL:
                sceneManager.parseNext();
                break;

            case SDLK_f:
                imageManager.toggle_fullscreen();
                break;

            case SDLK_1:
            case SDLK_2:
            case SDLK_3:
            case SDLK_4:
            case SDLK_5:
            case SDLK_6:
            case SDLK_7:
            case SDLK_8:
            case SDLK_9:
                if (SDL_GetModState() & KMOD_ALT && SDL_GetModState() & KMOD_SHIFT)
                    break;

                if (SDL_GetModState() & KMOD_ALT)
                    sceneManager.saveState(event.key.keysym.sym - SDLK_1);
                else if (SDL_GetModState() & KMOD_SHIFT)
                    sceneManager.loadState(event.key.keysym.sym - SDLK_1);
                else
                    sceneManager.selectChoice(event.key.keysym.sym - SDLK_1);
                break;

            default:
                break;
            }
            break;

        case SDL_QUIT:
            SDL_Quit();
            exit(0);

        default:
            break;
        }
    }
}

int main(int argc, char **argv)
{

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(main_loop, -1, 1);
#else
    for (;;)
        main_loop();
#endif

    return 0;
}
