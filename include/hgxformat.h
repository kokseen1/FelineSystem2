#pragma once

#include <asmodean.h>

typedef struct
{
    uint32 Width;
    uint32 Height;
    uint32 BitDepth;
    int32 OffsetX;
    int32 OffsetY;
    uint32 TotalWidth;
    uint32 TotalHeight;
    uint32 IsTransparent;
    int32 BaseX;
    int32 BaseY;
} Stdinfo;

typedef struct
{
    uint32 SliceStart;
    uint32 SliceLength;
    uint32 CompressedDataLength;
    uint32 DecompressedDataLength;
    uint32 CompressedCmdLength;
    uint32 DecompressedCmdLength;
} Img;

typedef struct FrameTag
{
    char TagName[8];
    uint32 OffsetNext;
    uint32 Length;
    union
    {
        Stdinfo Stdinfo;
        Img Img;
    };
} FrameTag;

typedef struct
{
    uint32 OffsetNext;
    uint32 ID;
} FrameHeader;

typedef struct
{
    char FileSignature[4];
    uint32 HeaderSize;
    uint32 Version;
} HGHeader;
