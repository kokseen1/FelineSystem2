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

class HGDecoder
{

public:
    // Holds pointers to structs necessary for decoding image
    typedef struct
    {
        Stdinfo *Stdinfo;
        Img *Img;
    } Frame;

    static SDL_Surface *getSurfaceFromFrame(Frame);

    static std::vector<Frame> getFrames(FrameHeader *);

private:
    static Frame getFrame(FrameTag *);

    static std::vector<byte> getPixelsFromFrame(Frame);

    static SDL_Surface *getSurfaceFromPixels(std::vector<byte>, Frame);

    static SDL_Surface *flip_vertical(SDL_Surface *);
};
