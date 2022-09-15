#include <stdio.h>
#include <string.h>
#include <hgdecoder.hpp>
#include <hgx2bmp.h>

#include <utils.hpp>

// Allocates and returns a flipped surface
// Frees the original surface
// Caller is responsible for freeing the returned surface
SDL_Surface *HGDecoder::flip_vertical(SDL_Surface *sfc)
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
    SDL_FreeSurface(sfc);
    return result;
}

// Decodes a frame and returns a vector of pixels rgbaBuffer
std::vector<byte> HGDecoder::getPixelsFromFrame(Frame frame)
{
    // Get locations of Data and Cmd
    byte *RleData = reinterpret_cast<byte *>(frame.Img + 1);
    byte *RleCmd = RleData + frame.Img->CompressedDataLength;

    // Zlib decompress
    auto RleDataDecompressed = Utils::zlibUncompress(frame.Img->DecompressedDataLength, RleData, frame.Img->CompressedDataLength);
    if (RleDataDecompressed.empty())
    {
        printf("RleData uncompress error\n");
        return {};
    }

    auto RleCmdDecompressed = Utils::zlibUncompress(frame.Img->DecompressedCmdLength, RleCmd, frame.Img->CompressedCmdLength);
    if (RleCmdDecompressed.empty())
    {
        printf("RleCmd uncompress error\n");
        return {};
    }

    // Calculate byte depth
    int depthBytes = BYTE_DEPTH(frame.Stdinfo->BitDepth);

    uint32 szRgbaBuffer = frame.Stdinfo->Width * frame.Stdinfo->Height * depthBytes;
    std::vector<byte> rgbaBuffer(szRgbaBuffer);
    // printf("Allocated %lu bytes\n", szRgbaBuffer);

    // Decode image
    ReturnCode ret = ProcessImage(&RleDataDecompressed[0], frame.Img->DecompressedDataLength, &RleCmdDecompressed[0], frame.Img->DecompressedCmdLength, &rgbaBuffer[0], szRgbaBuffer, frame.Stdinfo->Width, frame.Stdinfo->Height, depthBytes, STRIDE(frame.Stdinfo->Width, depthBytes));
    if (ReturnCode::Success != ret)
    {
        printf("ProcessImage error %lx\n", ret);
        return {};
    }

    return rgbaBuffer;
}

// Parses frame tags and returns a Frame struct pointing to the frame data
HGDecoder::Frame HGDecoder::getFrame(FrameTag *frameTag)
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
            // printf("Ignoring tag %s\n", frameTag->TagName);
        }

        // Reached last tag
        if (frameTag->OffsetNext == 0)
            break;

        // Shift by byte offset
        frameTag = reinterpret_cast<FrameTag *>(reinterpret_cast<byte *>(frameTag) + frameTag->OffsetNext);
    }

    return frame;
}

// Helper function to directly get surface from frame
// Caller is responsible for freeing the surface
SDL_Surface *HGDecoder::getSurfaceFromFrame(Frame frame)
{
    auto rgbaBuffer = getPixelsFromFrame(frame);
    SDL_Surface *surface = getSurfaceFromPixels(rgbaBuffer, frame);

    return surface;
}

// Returns an image surface in proper orientation from a rgbaBuffer vector
// Caller is responsible for freeing the surface
SDL_Surface *HGDecoder::getSurfaceFromPixels(std::vector<byte> rgbaBuffer, Frame frame)
{
    SDL_Surface *result = NULL;
    SDL_Surface *surface = SDL_CreateRGBSurfaceFrom(rgbaBuffer.data(), frame.Stdinfo->Width, frame.Stdinfo->Height, frame.Stdinfo->BitDepth, PITCH(frame.Stdinfo->Width, frame.Stdinfo->BitDepth), RMASK, GMASK, BMASK, AMASK);
    if (surface != NULL)
    {
        result = flip_vertical(surface);
    }

    return result;
}

// Get a vector of Frame structures that contain pointers to frame data
std::vector<HGDecoder::Frame> HGDecoder::getFrames(FrameHeader *frameHeader)
{
    std::vector<Frame> frames;

    while (1)
    {
        Frame frame = getFrame(reinterpret_cast<FrameTag *>(frameHeader + 1));
        frames.push_back(frame);

        // Reach last frame
        if (frameHeader->OffsetNext == 0)
        {
            break;
        }

        frameHeader = reinterpret_cast<FrameHeader *>((reinterpret_cast<byte *>(frameHeader) + frameHeader->OffsetNext));
    }

    return frames;
}
