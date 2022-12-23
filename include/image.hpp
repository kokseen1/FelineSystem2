#pragma once

#include <hgdecoder.hpp>
#include <file.hpp>
#include <window.hpp>
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
    EG,
    FG,
    CG,
    FW,
};

class ImageManager
{

public:
    std::string currText;
    std::string currSpeaker;

    ImageManager(FileManager &, SDL_Renderer *, std::vector<Choice> &);

    void setFrameon(const unsigned int);

    void setFrameoff(const unsigned int);

    void clearCanvas();

    void loadDump(const json &);

    const json dump();

    const std::pair<int, int> getShifts(const IMAGE_TYPE, const int);

    void setFade(const IMAGE_TYPE, const int, const unsigned int, const Uint8, const Uint8);

    void setMove(const IMAGE_TYPE, const int, const unsigned int, const int, const int);

    void setBlend(const IMAGE_TYPE, const int, const unsigned int);

    void setImage(const IMAGE_TYPE, const int, std::string, int, int);

    void clearImageType(const IMAGE_TYPE);

    void clearZIndex(const IMAGE_TYPE, const int);

    void render();

    void setShowMwnd();
    void setHideMwnd();
    void toggleMwnd() { showMwnd = !showMwnd; };

    void setShowText() { showText = true; };
    void setHideText() { showText = false; };

    SDL_Renderer *getRenderer() { return renderer; };

    TextureCache &getCache() { return textureCache; };

    FileManager &getFileManager() { return fileManager; };

    void processImage(byte *, size_t, const ImageData &);

    void killRdraw();

    void setRdraw(const unsigned int);

    Uint64 getFramestamp() { return framestamp; }
    Uint64 getRdrawStart() { return rdrawStart; }
    unsigned int getGlobalRdraw() { return globalRdraw; }

private:
    // Used for synchronizing transitions/animations with the render framerate
    Uint64 framestamp = 0;
    Uint64 rdrawStart = 0;
    unsigned int globalRdraw = 0;

    std::vector<Choice> &currChoices;

    TextureCache textureCache;

    FileManager &fileManager;

    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;

    bool showMwnd = true;
    bool showText = true;

    Image mwnd{*this, MWND, MWND_XSHIFT, MWND_YSHIFT, MWND_ALPHA};
    Image mwndDeco{*this, MWND_DECO, MWND_XSHIFT, MWND_YSHIFT};

    TTF_Font *font = NULL;
    TTF_Font *selectFont = NULL;

    SDL_Color textColor = {255, 255, 255, 0};

    // Arrays to simulate the current canvas with layers
    ImageLayer<Bg, MAX_BG> bgLayer;
    ImageLayer<Eg, MAX_EG> egLayer;
    ImageLayer<Cg, MAX_CG> cgLayer;
    ImageLayer<Fw, MAX_FW> fwLayer;
    ImageLayer<Fg, MAX_FG> fgLayer;

    SDL_Texture *getTextureFromFrame(HGDecoder::Frame);

    void renderChoices();

    void setLogo();

    void fetchImage(const Image &, const std::string &);

    void renderMessage(const std::string &);

    void renderSpeaker(const std::string &);
};
