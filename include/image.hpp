#pragma once

#include <hgdecoder.hpp>
#include <file.hpp>
#include <scene.hpp>

#include <asmodean.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <string>
#include <cstring>
#include <iostream>
#include <map>
#include <unordered_map>
#include <utility>
#include <array>

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
#define SELECT_FONT_PATH FONT_PATH
#define SELECT_FONT_SIZE 30

#define SEL "sys_sel_02"
#define SEL_WIDTH 782
#define SEL_HEIGHT 63
#define SEL_SPACING 30
#define SEL_XSHIFT WINDOW_WIDTH / 2 - SEL_WIDTH / 2

#define MWND "sys_mwnd_43"
#define MWND_DECO "sys_mwnd_42"
#define MWND_ALPHA 130

// Fixed position of message window
#define MWND_XSHIFT 100
#define MWND_YSHIFT 422

// Determines length before wrapping text
#define TEXTBOX_WIDTH 650

// Text is relative to center of window based on width
#define TEXT_XPOS (WINDOW_WIDTH - TEXTBOX_WIDTH) / 2 + 50
#define TEXT_YPOS WINDOW_HEIGHT - 120

// Speaker name is relative to text position
#define SPEAKER_XPOS TEXT_XPOS
#define SPEAKER_YPOS TEXT_YPOS - 25

#define MAX_BG 10
#define MAX_EG 10
#define MAX_CG 10
#define MAX_FW 10

#define FW_XSHIFT 90
#define FW_YSHIFT 160

#define KEY_BG "bg"
#define KEY_EG "eg"
#define KEY_CG "cg"
#define KEY_FW "fw"
#define KEY_NAME "name"
#define KEY_XSHIFT "x"
#define KEY_YSHIFT "y"

enum class IMAGE_TYPE
{
    BG,
    CG,
    EG,
    FW,
};

typedef struct
{
    int xShift;
    int yShift;
    std::vector<std::string> names;
} ImageData;

typedef std::pair<SDL_Texture *, Stdinfo> TextureData;

class Image
{
public:
    std::string textureName;

    // Additional offset applied on top of base position
    int xShift = 0;
    int yShift = 0;

    Image(){};

    Image(SDL_Renderer *, std::string, int, int);

    bool isActive() { return !textureName.empty(); }

    void render();

    void clear();

    const json dump();

protected:
    SDL_Renderer *renderer = NULL;

    void render(const int, const int);

private:
    TextureData *textureData = NULL;
};

class Choice : public Image
{
private:
    void renderText(const int, const int);

public:
    const std::string target;
    const std::string prompt;

    Choice(SDL_Renderer *, const std::string &, const std::string &);

    void render(const int);
};

class Bg : public Image
{
    // Inherit ctors
    using Image::Image;
};

class Eg : public Image
{
    // Inherit ctors
    using Image::Image;
};

class Cg
{
public:
    void render();

    void clear();

    bool isActive() { return !assetRaw.empty(); }

    const json dump();

    Image base;
    Image part1;
    Image part2;

    std::string assetRaw;
};

class Fw : public Cg
{
public:
    const json dump();
};

class ImageManager
{

public:
    std::string currText;
    std::string currSpeaker;

    ImageManager(FileManager *);

    void init(SceneManager *sm) { sceneManager = sm; }

    void clearCanvas();

    void loadDump(const json &);

    const json dump();

    const Image &getImage(const IMAGE_TYPE, const int);

    void setImage(const IMAGE_TYPE, const int, std::string, int, int);

    void clearImageType(const IMAGE_TYPE);

    void clearZIndex(const IMAGE_TYPE, const int);

    void render();

    void toggle_fullscreen();

    void toggleMwnd() { showMwnd = !showMwnd; };

    SDL_Renderer *getRenderer() { return renderer; };

private:
    FileManager *fileManager = NULL;
    SceneManager *sceneManager = NULL;

    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;

    bool showMwnd = true;

    Image mwnd;
    Image mwndDeco;

    TTF_Font *font = NULL;
    TTF_Font *selectFont = NULL;

    SDL_Color textColor = {255, 255, 255, 0};

    // Arrays to simulate the current canvas with layers
    std::array<Bg, MAX_BG> currBgs;
    std::array<Eg, MAX_EG> currEgs;
    std::array<Cg, MAX_CG> currCgs;
    std::array<Fw, MAX_FW> currFws;

    std::vector<std::string> getAssetArgs(const std::string &);

    SDL_Texture *getTextureFromFrame(HGDecoder::Frame);

    void renderChoices();

    void setWindowIcon(SDL_Window *);

    void setLogo();

    void fetchImage(const std::string &);

    void processImage(byte *, size_t, std::pair<std::string, int>);

    void renderImage(const ImageData &);

    void renderMessage(const std::string &);

    void renderSpeaker(const std::string &);
};
