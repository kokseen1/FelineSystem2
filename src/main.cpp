#include <stdio.h>
#include <SDL2/SDL.h>

#include <fstream>
#include <vector>

#include <music.hpp>
#include <image.hpp>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

int track_id = 0;
#ifdef __EMSCRIPTEN__

void onLoad(void *arg, void *buf, int sz)
{
    // buf will be freed after returning from this callback
    ((MusicPlayer *)arg)->playFromMem(buf, sz);
}

void onError(void *arg)
{
    printf("onError\n");
}

static inline const char *emscripten_event_type_to_string(int eventType)
{
    const char *events[] = {"(invalid)", "(none)", "keypress", "keydown", "keyup", "click", "mousedown", "mouseup", "dblclick", "mousemove", "wheel", "resize",
                            "scroll", "blur", "focus", "focusin", "focusout", "deviceorientation", "devicemotion", "orientationchange", "fullscreenchange", "pointerlockchange",
                            "visibilitychange", "touchstart", "touchend", "touchmove", "touchcancel", "gamepadconnected", "gamepaddisconnected", "beforeunload",
                            "batterychargingchange", "batterylevelchange", "webglcontextlost", "webglcontextrestored", "(invalid)"};
    ++eventType;
    if (eventType < 0)
        eventType = 0;
    if (eventType >= sizeof(events) / sizeof(events[0]))
        eventType = sizeof(events) / sizeof(events[0]) - 1;
    return events[eventType];
}

void nextTrack(void *userData)
{
    track_id++;
    if (track_id == 31)
        track_id = 1;

    char fpath[255] = {0};
    sprintf(fpath, TRACK, track_id);

    // for (int i = 0; i < 50; i++)
    emscripten_async_wget_data(fpath, userData, onLoad, onError);
}

EM_BOOL wheel_callback(int eventType, const EmscriptenWheelEvent *e, void *userData)
{
    nextTrack(userData);
    return 0;
}

EM_BOOL mouse_callback(int eventType, const EmscriptenMouseEvent *e, void *userData)
{
    // printf("%s, screen: (%ld,%ld), client: (%ld,%ld),%s%s%s%s button: %hu, buttons: %hu, movement: (%ld,%ld), target: (%ld, %ld)\n",
    //        emscripten_event_type_to_string(eventType), e->screenX, e->screenY, e->clientX, e->clientY,
    //        e->ctrlKey ? " CTRL" : "", e->shiftKey ? " SHIFT" : "", e->altKey ? " ALT" : "", e->metaKey ? " META" : "",
    //        e->button, e->buttons, e->movementX, e->movementY, e->targetX, e->targetY);

    if (e->screenX != 0 && e->screenY != 0 && e->clientX != 0 && e->clientY != 0 && e->targetX != 0 && e->targetY != 0)
    {
        if (eventType == EMSCRIPTEN_EVENT_CLICK)
        {
            nextTrack(userData);
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

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 1024) == -1)
    {
        printf("%s", Mix_GetError());
    }

    // Must dynamically allocate as variables in main are freed
    MusicPlayer *musicPlayer = new MusicPlayer();

    SDL_Window *window = NULL;
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_RESULT ret = emscripten_set_click_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, musicPlayer, 1, mouse_callback);
    ret = emscripten_set_wheel_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, musicPlayer, 1, wheel_callback);

    SDL_Renderer *renderer = NULL;
    SDL_CreateWindowAndRenderer(1024, 576, 0, &window, &renderer);
#else
    window = SDL_CreateWindow("FelineSystem2",
                              SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              1024,
                              576,
                              SDL_WINDOW_SHOWN);

    std::ifstream infile("assets/bgm01.ogg", std::ios_base::binary);
    std::vector<char> buffer((std::istreambuf_iterator<char>(infile)),
                             (std::istreambuf_iterator<char>()));
    musicPlayer->playFromMem(buffer.data(), buffer.size());

    std::ifstream hgfile("assets/bg09.hg3", std::ios_base::binary);
    std::vector<char> hgbuf((std::istreambuf_iterator<char>(hgfile)),
                            (std::istreambuf_iterator<char>()));
    HGDecoder::displayFromMem(hgbuf.data(), window);

    SDL_Delay(10000);
#endif

    return 0;
}
