#include <stdio.h>
#include <SDL2/SDL.h>

#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>

#include <music.hpp>
#include <image.hpp>
#include <scene.hpp>
#include <file.hpp>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

// Initialize manager classes
static FileManager fileManager;
static MusicManager musicManager(&fileManager);
static ImageManager imageManager(&fileManager);
static SceneManager sceneManager(&musicManager, &imageManager, &fileManager);

#ifdef __EMSCRIPTEN__

EM_BOOL wheel_callback(int eventType, const EmscriptenWheelEvent *e, void *userData)
{
    sceneManager.parseNext();
    return 0;
}

EM_BOOL mouse_callback(int eventType, const EmscriptenMouseEvent *e, void *userData)
{
    if (e->screenX != 0 && e->screenY != 0 && e->clientX != 0 && e->clientY != 0 && e->targetX != 0 && e->targetY != 0)
    {
        if (eventType == EMSCRIPTEN_EVENT_CLICK)
        {
            sceneManager.parseNext();
        }
    }

    return 0;
}

// The event handler functions can return 1 to suppress the event and disable the default action. That calls event.preventDefault();
// Returning 0 signals that the event was not consumed by the code, and will allow the event to pass on and bubble up normally.
EM_BOOL key_callback(int eventType, const EmscriptenKeyboardEvent *e, void *userData)
{
    if (eventType == EMSCRIPTEN_EVENT_KEYPRESS && (e->which >= 49 && e->which <= 57))
    {
        sceneManager.selectChoice(e->which - 49);
    }
    // if (eventType == EMSCRIPTEN_EVENT_KEYPRESS && (!strcmp(e->key, "k")))
    // LOG << "choice";
    if (eventType == EMSCRIPTEN_EVENT_KEYPRESS && (!strcmp(e->key, "f") || e->which == 102))
    {
        EmscriptenFullscreenChangeEvent fsce;
        EMSCRIPTEN_RESULT ret = emscripten_get_fullscreen_status(&fsce);
        if (!fsce.isFullscreen)
        {
            printf("Requesting fullscreen..\n");
            ret = emscripten_request_fullscreen("#canvas", 1);
        }
        else
        {
            printf("Exiting fullscreen..\n");
            ret = emscripten_exit_fullscreen();
            ret = emscripten_get_fullscreen_status(&fsce);
            if (fsce.isFullscreen)
            {
                fprintf(stderr, "Fullscreen exit did not work!\n");
            }
        }
    }

    return 0;
}

#endif

int main(int argc, char **argv)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
        return 1;
    }

#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_RESULT ret = emscripten_set_click_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, 1, mouse_callback);
    ret = emscripten_set_keypress_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, 1, key_callback);
    ret = emscripten_set_wheel_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, 1, wheel_callback);

    // emscripten_set_main_loop(nextTrack, 0, 1);

#else
    bool done = false;
    SDL_Event event;

    while ((!done) && (SDL_WaitEvent(&event)))
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
            case SDLK_1:
            case SDLK_2:
            case SDLK_3:
            case SDLK_4:
            case SDLK_5:
            case SDLK_6:
            case SDLK_7:
            case SDLK_8:
            case SDLK_9:
                sceneManager.selectChoice(event.key.keysym.sym - SDLK_1);
                break;
            default:
                break;
            }
            break;

        case SDL_QUIT:
            done = true;
            SDL_Quit();
            break;

        default:
            break;
        }
    }

#endif
    return 0;
}
