#pragma once

#include <hgdecoder.hpp>
#include <utils.hpp>
#include <window.hpp>

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
    // Name of base asset
    std::string baseName;

    // Additional offset applied on top of base position
    int xShift = 0;
    int yShift = 0;

    Image(SDL_Renderer *, TextureCache &);

    Image(SDL_Renderer *, TextureCache &, std::string, int, int);

    bool isActive() { return !baseName.empty(); }

    void set(const std::string &, int, int);

    void render();

    void clear();

    const json dump();

    void render(const int, const int);

protected:
    TextureCache &textureCache;

    SDL_Renderer *renderer;
};

class Choice : public Image
{
private:
    void renderText(const int, const int);

public:
    const std::string target;
    const std::string prompt;

    Choice(SDL_Renderer *, TextureCache &, const std::string &, const std::string &);

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

class Cg : public Image
{
public:
    Image part1;
    Image part2;

    std::string assetRaw;

    Cg(SDL_Renderer *, TextureCache &);

    void render();

    void clear();

    const json dump();
};

class Fw : public Cg
{
    using Cg::Cg;

public:
    void render();

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
    ImageLayer(SDL_Renderer *renderer, TextureCache &textureCache) : objects{Utils::create_array<_Nm, _Tp>({renderer, textureCache})} {}

    _Tp &operator[](size_t i) { return objects[i]; }

    constexpr size_t size() { return objects.size(); }

    const std::pair<int, int> getShifts(size_t i)
    {
        if (i >= size())
            throw std::runtime_error("Out of range access");
        return {objects[i].xShift, objects[i].yShift};
    }

    // const json dump()
    // {
    //     json j;
    //     for (int i = 0; i < size(); i++)
    //     {
    //         if (!objects[i].isActive())
    //             continue;
    //         j[std::to_string(i)] = objects[i].dump();
    //     }
    //     return j;
    // }

    void clear(size_t i)
    {
        if (i >= size())
            return;
        objects[i].clear();
    }

    void clear()
    {
        for (auto &image : objects)
            image.clear();
    }

    void render()
    {
        for (auto &image : objects)
            image.render();
    }
};
