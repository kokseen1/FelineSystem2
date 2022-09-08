// Copyright 2011 The Emscripten Authors.  All rights reserved.
// Emscripten is available under two separate licenses, the MIT license and the
// University of Illinois/NCSA Open Source License.  Both these licenses can be
// found in the LICENSE file.

#include <stdio.h>
#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

Mix_Music *mus = NULL;

void onLoad(void *arg, void *buf, int sz)
{
  // buf will be freed after returning from this callback
  // void* buffer = new char[sz];
  // SDL_memcpy(buffer, buf, sz);
  SDL_RWops *rw = SDL_RWFromConstMem(buf, sz);
  mus = Mix_LoadMUS_RW(rw);
  Mix_PlayMusic(mus, -1);
}

void onError(void *arg)
{
  printf("onError\n");
}

int main(int argc, char **argv)
{
  printf("hello\n");
  SDL_Init(SDL_INIT_EVERYTHING);
  SDL_Surface *screen = SDL_SetVideoMode(256, 256, 32, SDL_SWSURFACE);

  emscripten_async_wget_data("assets/bgm01.ogg", NULL, onLoad, onError);

  return 0;
}
