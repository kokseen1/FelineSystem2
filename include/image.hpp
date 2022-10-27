#pragma once

#include <hgdecoder.hpp>
#include <file.hpp>

#include <asmodean.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <string>
#include <cstring>
#include <iostream>
#include <map>
#include <utility>

#define IMAGE_EXT ".hg3"
#define IMAGE_SIGNATURE "HG-3"

// Flip mode when rendering image assets
#define RENDERER_FLIP_MODE SDL_FLIP_VERTICAL

#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 576

// #define WINDOW_WIDTH 1280
// #define WINDOW_HEIGHT 720

#define FONT_PATH ASSETS "font.ttf"
#define FONT_SIZE 20

// Determines length before wrapping text
#define TEXTBOX_WIDTH 650

// Centered on window based on width
#define TEXT_XPOS (WINDOW_WIDTH - TEXTBOX_WIDTH) / 2
#define TEXT_YPOS WINDOW_HEIGHT - 120

// Relative to text position
#define SPEAKER_XPOS TEXT_XPOS + 30
#define SPEAKER_YPOS TEXT_YPOS - 25

#define Z_INDEX_MAX 10

enum class IMAGE_TYPE
{
    IMAGE_EG,
    IMAGE_BG,
    IMAGE_CG,
    IMAGE_FW,
};

typedef struct
{
    int xShift;
    int yShift;
    std::vector<std::string> names;
} ImageData;

typedef std::pair<SDL_Texture *, Stdinfo> TextureData;

class ImageManager
{

public:
    std::string currText;
    std::string currSpeaker;

    ImageManager(FileManager *);

    void setImage(IMAGE_TYPE, int, std::string, int, int);

    void clearAllImage(IMAGE_TYPE);

    void clearZIndex(IMAGE_TYPE, int);

    void displayAll();

private:
    FileManager *fileManager = NULL;

    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;

    TTF_Font *font;
    SDL_Color textColor = {255, 255, 255, 0};

    ImageData currCgs[Z_INDEX_MAX];
    ImageData currBgs[Z_INDEX_MAX];
    ImageData currEgs[Z_INDEX_MAX];
    ImageData currFws[Z_INDEX_MAX];

    std::map<std::string, TextureData> textureDataCache;

    SDL_Texture *getTextureFromFrame(HGDecoder::Frame);

    void renderTexture(SDL_Texture *, int, int);

    void processImage(byte *, size_t, std::pair<std::string, int>);

    void renderImage(const ImageData &);

    void renderMessageWindow();

    void renderMessage(const std::string &);

    void renderSpeaker(const std::string &);
};
