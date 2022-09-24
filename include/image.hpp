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

enum class IMAGE_TYPE
{
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
    ARG_BG_Z_INDEX = 1,
    ARG_CG_Z_INDEX = 1,
    ARG_BG_NAME = 2,
    ARG_CG_NAME = 2,
    ARG_CG_BODY = 3,
    ARG_CG_EYES = 5,
    ARG_CG_MOUTH = 6
};

typedef struct
{
    // const std::vector<std::string> args;
    IMAGE_TYPE type;
    // IMAGE_SUBTYPE subtype;
    std::vector<std::string> names;
    int nameIdx;
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
    SDL_Texture *currentBg = NULL;

    // Map of image_name : texture
    std::map<std::string, TextureData> textureDataCache;

    SDL_Texture *getTextureFromFrame(HGDecoder::Frame);

    TextureData getCachedTexture(std::string);

    void queueImage(ImageData);

    void renderTexture(SDL_Texture *, int, int);

    void displayTexture(SDL_Texture *, ImageData);

    void processImage(byte *, size_t, ImageData);

    void displayImage(ImageData);
};
