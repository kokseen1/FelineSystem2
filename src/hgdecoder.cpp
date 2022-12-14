#include <hgdecoder.hpp>
#include <utils.hpp>
#include <hgx2bmp.h>

#include <stdio.h>
#include <string.h>

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
        LOG << "RleData uncompress error";
        return {};
    }

    auto RleCmdDecompressed = Utils::zlibUncompress(frame.Img->DecompressedCmdLength, RleCmd, frame.Img->CompressedCmdLength);
    if (RleCmdDecompressed.empty())
    {
        LOG << "RleCmd uncompress error";
        return {};
    }

    // Calculate byte depth
    int depthBytes = BYTE_DEPTH(frame.Stdinfo->BitDepth);

    uint32 szRgbaBuffer = frame.Stdinfo->Width * frame.Stdinfo->Height * depthBytes;
    if (szRgbaBuffer < 1024)
    {
        szRgbaBuffer = 1024;
    }
    std::vector<byte> rgbaBuffer(szRgbaBuffer);

    // Decode image
    ReturnCode ret = ProcessImage(&RleDataDecompressed[0], frame.Img->DecompressedDataLength, &RleCmdDecompressed[0], frame.Img->DecompressedCmdLength, &rgbaBuffer[0], szRgbaBuffer, frame.Stdinfo->Width, frame.Stdinfo->Height, depthBytes, STRIDE(frame.Stdinfo->Width, depthBytes));
    if (ReturnCode::Success != ret)
    {
        LOG << "ProcessImage error " << static_cast<int>(ret);
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
        // LOG << "Found tag " << frameTag->TagName;
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
            // Ignore tag
            // LOG << "Ignoring tag " << frameTag->TagName;
        }

        // Reached last tag
        if (frameTag->OffsetNext == 0)
            break;

        // Shift by byte offset
        frameTag = reinterpret_cast<FrameTag *>(reinterpret_cast<byte *>(frameTag) + frameTag->OffsetNext);
    }

    return frame;
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
