#pragma once

#include <SDL2/SDL.h>
#include <asmodean.h>

#include <string>
#include <cstring>
#include <iostream>
#include <map>

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
    IMAGE_CG_FACE,
};

enum ARG
{
    ARG_BG_Z_INDEX = 1,
    ARG_CG_Z_INDEX = 1,
    ARG_BG_NAME = 2,
    ARG_CG_NAME = 2,
    ARG_CG_SPRITE = 3,
    ARG_CG_FACE = 5
};

typedef struct
{
    const std::vector<std::string> args;
    const IMAGE_TYPE type;
    IMAGE_SUBTYPE subtype;
    std::string name;
} ImageData;

class ImageManager
{

public:
    ImageManager();

    void setImage(ImageData);

private:
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    SDL_Texture *currentBg = NULL;

    // Map of image_name : texture
    std::map<std::string, SDL_Texture *> textureCache;

    SDL_Texture *getTextureFromFrame(HGDecoder::Frame);

    SDL_Texture *getCachedTexture(std::string);

    void queueImage(ImageData);

    void renderTexture(SDL_Texture *, int, int);

    void displayTexture(SDL_Texture *, ImageData);

    void processImage(byte *, size_t, ImageData);
};
