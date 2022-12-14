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

class ImageManager;

class Image
{
public:
    // Name of base asset
    std::string baseName;

    // Additional offset applied on top of base position
    int xShift = 0;
    int yShift = 0;

    Image(ImageManager &, std::string = "", int = 0, int = 0);

    bool isActive() { return !baseName.empty(); }

    const std::pair<int, int> getShifts() { return {xShift, yShift}; }

    void fetch();

    void update(const std::string &, int, int);

    void set(const std::string &, int, int);

    void render();

    void clear();

    const json dump();

    void render(const int, const int);

protected:
    virtual const std::string getRawName() { return baseName; }

    ImageManager &imageManager;

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

    Choice(ImageManager &, const std::string &, const std::string &);

    void render();
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

    std::string rawName;

    Cg(ImageManager &);

    void render();

    void clear();

    void update(const std::string &, int, int);

protected:
    const std::string getRawName() { return rawName; }
};

class Fw : public Cg
{
    using Cg::Cg;

public:
    void render();
};

typedef struct
{
    const std::string name; // Cannot be a reference in async
    const int index;
    const Image *image;
} ImageData;

// Templated wrapper class for array of image objects
template <typename _Tp, std::size_t _Nm>
class ImageLayer
{
private:
    std::array<_Tp, _Nm> objects;

public:
    ImageLayer(ImageManager &imageManager) : objects{Utils::create_array<_Nm, _Tp>({imageManager})} {}

    _Tp &operator[](size_t i) { return objects[i]; }

    constexpr size_t size() { return objects.size(); }

    void update(const int i, std::string rawName, const int x, const int y)
    {
#ifdef LOWERCASE_ASSETS
        // Convert names to lowercase
        Utils::lowercase(rawName);
#endif

        if (i >= size())
            throw std::runtime_error("Out of range access");
        objects[i].update(rawName, x, y);
    }

    const std::pair<int, int> getShifts(size_t i)
    {
        if (i >= size())
            throw std::runtime_error("Out of range access");
        return objects[i].getShifts();
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

    void load(const json &j)
    {
        for (auto &e : j.items())
        {
            const auto i = std::stoi(e.key());
            if (i >= size())
                throw std::runtime_error("Out of range access");

            const auto &rawName = e.value().at(KEY_NAME).get<std::string>();
            auto xShift = e.value().at(KEY_XSHIFT).get<int>();
            auto yShift = e.value().at(KEY_YSHIFT).get<int>();

            objects[i].update(rawName, xShift, yShift);
        }
    }

    void clear(size_t i)
    {
        if (i >= size())
            throw std::runtime_error("Out of range access");
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
