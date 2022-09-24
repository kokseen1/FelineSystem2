#pragma once

#include <SDL2/SDL.h>
#include <asmodean.h>

#include <string>
#include <cstring>
#include <iostream>
#include <map>
#include <utility>

#include <hgdecoder.hpp>

#define IMAGE_PATH "image/"
#define IMAGE_EXT ".hg3"
#define IMAGE_SIGNATURE "HG-3"

#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 576

#define MAX_Z_INDEX 10

enum class IMAGE_TYPE
{
    IMAGE_EG,
    IMAGE_BG,
    IMAGE_CG,
};

enum class IMAGE_SUBTYPE
{
    IMAGE_CG_SPRITE,
    IMAGE_CG_EYES,
    IMAGE_CG_MOUTH,
};

enum ARG
{
    ARG_EG_Z_INDEX = 1,
    ARG_BG_Z_INDEX = 1,
    ARG_CG_Z_INDEX = 1,
    ARG_EG_NAME = 2,
    ARG_BG_NAME = 2,
    ARG_CG_NAME = 2,
    ARG_CG_BODY = 3,
    ARG_CG_EYES = 5,
    ARG_CG_MOUTH = 6,
    ARG_CG_SPEC = 7,
};

typedef struct
{
    IMAGE_TYPE type;
    std::vector<std::string> names;
    int nameIdx;
    int xShift;
    int yShift;
} ImageData;

typedef std::pair<SDL_Texture *, Stdinfo> TextureData;

class ImageManager
{

public:
    ImageManager();

    void setImage(std::vector<std::string>, IMAGE_TYPE);

private:
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;

    ImageData currSprites[MAX_Z_INDEX];
    ImageData currBgs[MAX_Z_INDEX];
    ImageData currEgs[MAX_Z_INDEX];

    std::map<std::string, TextureData> textureDataCache;

    SDL_Texture *getTextureFromFrame(HGDecoder::Frame);

    void renderTexture(SDL_Texture *, int, int);

    void processImage(byte *, size_t, ImageData);

    void displayImage(ImageData);

    void displayAll();
};
