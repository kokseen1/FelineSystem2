#pragma once

#include <SDL2/SDL.h>
#include <hgxformat.h>

#include <vector>

#define RMASK 0x00ff0000
#define GMASK 0x0000ff00
#define BMASK 0x000000ff
#define AMASK 0xff000000

#define BYTE_DEPTH(bits) ((bits + 7) / 8)
#define STRIDE(width, bytes) ((width * bytes + 3) & ~3)
#define PITCH(width, bits) (width * BYTE_DEPTH(bits))

// Holds pointers to structs necessary for decoding image
typedef struct
{
    Stdinfo *Stdinfo;
    Img *Img;
} Frame;

class HGDecoder
{

public:
    static void displayFromMem(void *, SDL_Renderer *);

    static void displayFromMem(void *, SDL_Window *);

    static SDL_Surface *getSurfaceFromFrame(Frame);

    static std::vector<Frame> getFrames(FrameHeader *);

private:
    static Frame getFrame(FrameTag *);

    static byte *getPixelsFromFrame(Frame);

    static SDL_Surface *getSurfaceFromPixels(byte *, Frame);
};

SDL_Surface *flip_vertical(SDL_Surface *);
