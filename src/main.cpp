#include <stdio.h>
#include <SDL2/SDL.h>

#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>

#include <music.hpp>
#include <scene.hpp>
#include <parser.hpp>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

static MusicPlayer musicPlayer;
static ImageManager imageManager;
static ScriptParser scriptParser(&musicPlayer, &imageManager);

void nextScene()
{
    scriptParser.parseNext();
    // static int scene_id = 1;
    // std::stringstream ss;
    // ss << "BG" << std::setfill('0') << std::setw(2) << std::to_string(scene_id);
    // imageManager.setImage(ImageData{ss.str(), IMAGE_TYPE::IMAGE_BG});
    // scene_id = scene_id == 11 ? 1 : scene_id + 1;
}

void nextTrack()
{
    scriptParser.parseNext();
    // static int track_id = 1;
    // std::stringstream ss;
    // ss << ASSETS "/bgm/bgm" << std::setfill('0') << std::setw(2) << std::to_string(track_id) << ".ogg";
    // musicPlayer.setMusic(ss.str());
    // track_id = track_id == 9 ? 1 : track_id + 1;
}

#ifdef __EMSCRIPTEN__

EM_BOOL wheel_callback(int eventType, const EmscriptenWheelEvent *e, void *userData)
{
    nextScene();
    return 0;
}

EM_BOOL mouse_callback(int eventType, const EmscriptenMouseEvent *e, void *userData)
{
    if (e->screenX != 0 && e->screenY != 0 && e->clientX != 0 && e->clientY != 0 && e->targetX != 0 && e->targetY != 0)
    {
        if (eventType == EMSCRIPTEN_EVENT_CLICK)
        {
            nextTrack();
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
            nextScene();
            break;

        case SDL_MOUSEBUTTONDOWN:
            nextTrack();
            break;

        case SDL_QUIT:
            done = true;
            break;

        default:
            break;
        }
    }

#endif
    return 0;
}
