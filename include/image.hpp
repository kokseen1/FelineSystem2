#pragma once

#include <hgdecoder.hpp>
#include <file.hpp>
#include <scene.hpp>
#include <imgtypes.hpp>

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

enum class IMAGE_TYPE
{
    BG,
    CG,
    EG,
    FW,
};

class ImageManager
{

public:
    std::string currText;
    std::string currSpeaker;

    ImageManager(FileManager &);

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

    SDL_Renderer *&getRenderer() { return renderer; };

    TextureCache &getCache() { return textureCache; };

private:
    TextureCache textureCache;

    FileManager &fileManager;
    SceneManager *sceneManager = NULL;

    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;

    bool showMwnd = true;

    Image mwnd{renderer, textureCache};
    Image mwndDeco{renderer, textureCache};

    TTF_Font *font = NULL;
    TTF_Font *selectFont = NULL;

    SDL_Color textColor = {255, 255, 255, 0};

    // Arrays to simulate the current canvas with layers
    ImageLayer<Bg, MAX_BG> bgLayer;
    ImageLayer<Eg, MAX_EG> egLayer;
    ImageLayer<Cg, MAX_CG> cgLayer;
    ImageLayer<Fw, MAX_FW> fwLayer;

    std::vector<std::string> getAssetArgs(const std::string &);

    SDL_Texture *getTextureFromFrame(HGDecoder::Frame);

    void renderChoices();

    void setWindowIcon(SDL_Window *);

    void setLogo();

    void fetchImage(const Image &, const std::string &);

    void processImageData(byte *, size_t, const ImageData &);

    void processImage(byte *, size_t, std::pair<std::string, int>);

    void renderMessage(const std::string &);

    void renderSpeaker(const std::string &);
};
