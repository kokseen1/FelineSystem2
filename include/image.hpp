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

#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 576

#define TEXTBOX_WIDTH 650

#define TEXT_XPOS (WINDOW_WIDTH - TEXTBOX_WIDTH) / 2
#define TEXT_YPOS 450

#define SPEAKER_XPOS TEXT_XPOS + 30
#define SPEAKER_YPOS 430

#define FONT_PATH ASSETS "font.ttf"
#define FONT_SIZE 20

#define Z_INDEX_MAX 10

enum class IMAGE_TYPE
{
    IMAGE_EG,
    IMAGE_BG,
    IMAGE_CG,
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

    std::map<std::string, TextureData> textureDataCache;

    SDL_Texture *getTextureFromFrame(HGDecoder::Frame);

    void renderTexture(SDL_Texture *, int, int);

    void processImage(byte *, size_t, std::pair<std::string, int>);

    void renderImage(ImageData);

    void renderImages();

    void renderText(std::string);

    void renderSpeaker(const std::string &);
};
