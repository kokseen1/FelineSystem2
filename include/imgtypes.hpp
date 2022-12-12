#pragma once

#include <hgdecoder.hpp>
#include <utils.hpp>

#include <SDL2/SDL.h>

#define KEY_BG "bg"
#define KEY_EG "eg"
#define KEY_CG "cg"
#define KEY_FW "fw"
#define KEY_NAME "name"
#define KEY_XSHIFT "x"
#define KEY_YSHIFT "y"

#define FW_XSHIFT 90
#define FW_YSHIFT 160

#define MAX_BG 10
#define MAX_EG 10
#define MAX_CG 10
#define MAX_FW 10

#define FONT_PATH ASSETS "font.ttf"
#define FONT_SIZE 20
#define SELECT_FONT_PATH FONT_PATH
#define SELECT_FONT_SIZE 30

#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 576

// #define WINDOW_WIDTH 1280
// #define WINDOW_HEIGHT 720

#define SEL "sys_sel_02"
#define SEL_WIDTH 782
#define SEL_HEIGHT 63
#define SEL_SPACING 30
#define SEL_XSHIFT WINDOW_WIDTH / 2 - SEL_WIDTH / 2

// Flip mode when rendering image assets
#define RENDERER_FLIP_MODE SDL_FLIP_VERTICAL

typedef std::pair<SDL_Texture *, Stdinfo> TextureData;
typedef std::unordered_map<std::string, TextureData> TextureCache;

class Image
{
public:
    std::string textureName;

    // Additional offset applied on top of base position
    int xShift = 0;
    int yShift = 0;

    Image(){};

    Image(SDL_Renderer *, TextureCache *, std::string, int, int);

    bool isActive() { return !textureName.empty(); }

    void render();

    void clear();

    const json dump();

protected:
    TextureCache *textureCache = NULL;

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

    Choice(SDL_Renderer *, TextureCache *, const std::string &, const std::string &);

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

typedef struct
{
    const Image &image;
    const std::string name; // Cannot be a reference in async
    const int index;
} ImageData;

// Templated wrapper class for array of image objects
template <typename _Tp, std::size_t _Nm>
class ImageLayer
{
private:
    std::array<_Tp, _Nm> objects;

public:
    _Tp &operator[](size_t i) { return objects[i]; }

    constexpr size_t size() { return objects.size(); }

    const _Tp &get(size_t i)
    {
        if (i >= size())
            return {};
        return objects[i];
    }

    const json dump()
    {
        json j;
        for (int i = 0; i < size(); i++)
        {
            if (!objects[i].isActive())
                continue;
            j[std::to_string(i)] = objects[i].dump();
        }
        return j;
    }

    void clear(size_t i)
    {
        if (i >= size())
            return;
        objects[i].clear();
    }

    void clear()
    {
        for (_Tp &image : objects)
            image.clear();
    }

    void render()
    {
        for (_Tp &image : objects)
            image.render();
    }
};
