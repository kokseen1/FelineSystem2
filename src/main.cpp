#include <audio.hpp>
#include <window.hpp>
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

static std::vector<Choice> currChoices;

static FileManager fileManager;
static WindowManager windowManager;
static AudioManager audioManager(fileManager);
static ImageManager imageManager(fileManager, windowManager.getRenderer(), currChoices);
static SceneManager sceneManager(audioManager, imageManager, fileManager, currChoices);

void main_loop()
{
    SDL_Event event;

    sceneManager.tickScript();

    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_USEREVENT:
            break;

        case SDL_MOUSEWHEEL:
            sceneManager.parse();
            break;

        case SDL_MOUSEBUTTONDOWN:
            sceneManager.parse();
            break;

        case SDL_KEYDOWN:
            switch (event.key.keysym.sym)
            {
            case SDLK_LCTRL:
            case SDLK_RETURN:
                sceneManager.parse();
                break;
            case SDLK_n:
                if (SDL_GetModState() & KMOD_SHIFT)
                    sceneManager.prevScene();
                else
                    sceneManager.nextScene();
                break;
            case SDLK_b:
                sceneManager.back();
                break;

            case SDLK_SPACE:
                imageManager.toggleMwnd();
                break;

            case SDLK_f:
                windowManager.toggleFullscreen();
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
                // Ensure compatibility with mobile browsers
                if (SDL_GetModState() & KMOD_ALT && SDL_GetModState() & KMOD_SHIFT)
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

    // Render the canvas
    imageManager.render();
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
