#include <stdio.h>
#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

#define TRACK "assets/bgm%02d.ogg"

Mix_Music *mus = NULL;
int track_id = 0;

void onLoad(void *arg, void *buf, int sz)
{
  // buf will be freed after returning from this callback
  // void* buffer = new char[sz];
  // SDL_memcpy(buffer, buf, sz);

  // Seems to help free memory
  if (mus != NULL)
  {
    Mix_HaltMusic();
    Mix_FreeMusic(mus);
  }

  SDL_RWops *rw = SDL_RWFromConstMem(buf, sz);
  // Above function is freed upon returning

  mus = Mix_LoadMUS_RW(rw);
  // Mix_Music *mus = Mix_LoadMUSType_RW(rw, MUS_OGG, 1);
  Mix_PlayMusic(mus, -1);

  printf("rw: %p\n", rw);
  printf("mus: %p\n", mus);
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

EM_BOOL mouse_callback(int eventType, const EmscriptenMouseEvent *e, void *renderer)
{
  // printf("%s, screen: (%ld,%ld), client: (%ld,%ld),%s%s%s%s button: %hu, buttons: %hu, movement: (%ld,%ld), target: (%ld, %ld)\n",
  //        emscripten_event_type_to_string(eventType), e->screenX, e->screenY, e->clientX, e->clientY,
  //        e->ctrlKey ? " CTRL" : "", e->shiftKey ? " SHIFT" : "", e->altKey ? " ALT" : "", e->metaKey ? " META" : "",
  //        e->button, e->buttons, e->movementX, e->movementY, e->targetX, e->targetY);

  if (e->screenX != 0 && e->screenY != 0 && e->clientX != 0 && e->clientY != 0 && e->targetX != 0 && e->targetY != 0)
  {
    if (eventType == EMSCRIPTEN_EVENT_CLICK)
    {
      if (e->ctrlKey)
      {
        track_id--;
      }
      else
      {
        track_id++;
      }

      if (mus != NULL)
      {
        Mix_HaltMusic();
        Mix_FreeMusic(mus);
        return 0;
      }

      char fpath[255] = {0};
      sprintf(fpath, TRACK, track_id);
      printf("%s\n", fpath);
      // for (int i = 0; i < 50; i++)
      emscripten_async_wget_data(fpath, NULL, onLoad, onError);
    }
  }

  return 0;
}

int main(int argc, char **argv)
{
  SDL_Init(SDL_INIT_EVERYTHING);
  Mix_Init(MIX_INIT_OGG);
  SDL_Surface *screen = SDL_SetVideoMode(256, 256, 32, SDL_SWSURFACE);

  EMSCRIPTEN_RESULT ret = emscripten_set_click_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, screen, 1, mouse_callback);

  return 0;
}
