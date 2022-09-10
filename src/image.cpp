#include <stdio.h>
#include <string.h>
#include <image.hpp>
#include <hgx2bmp.h>

#include "zlib.h"

// Allocates and returns a flipped surface
SDL_Surface *flip_vertical(SDL_Surface *sfc)
{
    SDL_Surface *result = SDL_CreateRGBSurface(sfc->flags, sfc->w, sfc->h,
                                               sfc->format->BytesPerPixel * 8, sfc->format->Rmask, sfc->format->Gmask,
                                               sfc->format->Bmask, sfc->format->Amask);
    const auto pitch = sfc->pitch;
    const auto pxlength = pitch * (sfc->h - 1);
    auto pixels = static_cast<unsigned char *>(sfc->pixels) + pxlength;
    auto rpixels = static_cast<unsigned char *>(result->pixels);
    for (auto line = 0; line < sfc->h; ++line)
    {
        memcpy(rpixels, pixels, pitch);
        pixels -= pitch;
        rpixels += pitch;
    }
    return result;
}

// Decodes a frame and returns a pixel array rgbaBuffer
// Caller is responsible for freeing the buffer
byte *HGDecoder::getPixelsFromFrame(Frame frame)
{
    // Get locations of Data and Cmd
    byte *RleData = &frame.Img->DataStart;
    byte *RleCmd = RleData + frame.Img->CompressedDataLength;

    // Allocate memory for decompressed buffer
    byte *RleDataDecompressed = (byte *)malloc(frame.Img->DecompressedDataLength);
    byte *RleCmdDecompressed = (byte *)malloc(frame.Img->DecompressedCmdLength);

    // Zlib decompress
    int res = uncompress(RleDataDecompressed, &frame.Img->DecompressedDataLength, RleData, frame.Img->CompressedDataLength);
    if (Z_OK != res)
    {
        printf("uncompress error\n");
        return NULL;
    }

    res = uncompress(RleCmdDecompressed, &frame.Img->DecompressedCmdLength, RleCmd, frame.Img->CompressedCmdLength);
    if (Z_OK != res)
    {
        printf("uncompress error\n");
        return NULL;
    }

    // Calculate bit depth
    int depthBytes = BYTE_DEPTH(frame.Stdinfo->BitDepth);

    // Allocate memory for rgbaBuffer
    uint32 szRgbaBuffer = frame.Stdinfo->Width * frame.Stdinfo->Height * depthBytes;
    byte *rgbaBuffer = (byte *)malloc(szRgbaBuffer);
    printf("Allocated %lu bytes\n", szRgbaBuffer);

    // Decode image
    ReturnCode ret = ProcessImage(RleDataDecompressed, frame.Img->DecompressedDataLength, RleCmdDecompressed, frame.Img->DecompressedCmdLength, rgbaBuffer, szRgbaBuffer, frame.Stdinfo->Width, frame.Stdinfo->Height, depthBytes, STRIDE(frame.Stdinfo->Width, depthBytes));
    if (ReturnCode::Success != ret)
    {
        printf("ProcessImage error %lx\n", ret);
        return NULL;
    }

    free(RleDataDecompressed);
    free(RleCmdDecompressed);

    return rgbaBuffer;
}

// Parses frame tags and returns a Frame struct pointing to the frame data
Frame HGDecoder::getFrame(FrameTag *frameTag)
{
    Frame frame;

    while (1)
    {
        if (!strcmp(frameTag->TagName, "stdinfo"))
        {
            // Store pointer to Stdinfo
            frame.Stdinfo = &frameTag->Stdinfo;
        }
        else if (!strcmp(frameTag->TagName, "img0000"))
        {
            // Store pointer to Img
            frame.Img = &frameTag->Img;
        }
        else
        {
            printf("Ignoring tag %s\n", frameTag->TagName);
        }

        // Reached last tag
        if (frameTag->OffsetNext == 0)
            break;

        // Shift by byte offset
        frameTag = (FrameTag *)((char *)frameTag + frameTag->OffsetNext);
    }

    return frame;
}

// Helper function to directly get surface from frame
// Caller is responsible for freeing the surface
SDL_Surface *HGDecoder::getSurfaceFromFrame(Frame frame)
{
    byte *rgbaBuffer = getPixelsFromFrame(frame);
    SDL_Surface *flipped = getSurfaceFromPixels(rgbaBuffer, frame);

    free(rgbaBuffer);

    return flipped;
}

// Returns an image surface in proper orientation from a rgbaBuffer
// Caller is responsible for freeing the surface
SDL_Surface *HGDecoder::getSurfaceFromPixels(byte *rgbaBuffer, Frame frame)
{
    SDL_Surface *result = NULL;
    SDL_Surface *surface = SDL_CreateRGBSurfaceFrom(rgbaBuffer, frame.Stdinfo->Width, frame.Stdinfo->Height, frame.Stdinfo->BitDepth, PITCH(frame.Stdinfo->Width, frame.Stdinfo->BitDepth), RMASK, GMASK, BMASK, AMASK);
    if (surface != NULL)
    {
        result = flip_vertical(surface);
    }

    return result;
}

std::vector<Frame> HGDecoder::getFrames(FrameHeader *frameHeader)
{
    std::vector<Frame> frames;

    while (1)
    {
        Frame frame = getFrame(&frameHeader->FrameTagStart);
        frames.push_back(frame);

        // Reach last frame
        if (frameHeader->OffsetNext == 0)
        {
            break;
        }

        frameHeader = (FrameHeader *)((char *)frameHeader + frameHeader->OffsetNext);
    }

    return frames;
}

void HGDecoder::displayFromMem(void *hgHeader, SDL_Renderer *renderer)
{
    for (auto frame : HGDecoder::getFrames(&((HGHeader *)hgHeader)->FrameHeaderStart))
    {
        SDL_Surface *surface = HGDecoder::getSurfaceFromFrame(frame);

        SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
    }
}

void HGDecoder::displayFromMem(void *hgHeader, SDL_Window *window)
{
    for (auto frame : HGDecoder::getFrames(&((HGHeader *)hgHeader)->FrameHeaderStart))
    {
        SDL_Surface *surface = HGDecoder::getSurfaceFromFrame(frame);

        SDL_Surface *screen = SDL_GetWindowSurface(window);
        SDL_BlitSurface(surface, NULL, screen, NULL);
        SDL_UpdateWindowSurface(window);

        SDL_FreeSurface(surface);
    }
}
