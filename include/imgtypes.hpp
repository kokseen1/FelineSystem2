#pragma once

#include <hgdecoder.hpp>
#include <utils.hpp>
#include <window.hpp>

#include <SDL2/SDL.h>

#define KEY_BG "bg"
#define KEY_EG "eg"
#define KEY_FG "fg"
#define KEY_CG "cg"
#define KEY_FW "fw"
#define KEY_NAME "name"
#define KEY_XSHIFT "x"
#define KEY_YSHIFT "y"

#define MAX_ALPHA 255

#define FW_XSHIFT 90
#define FW_YSHIFT 160
#define FG_YSHIFT -400

#define MAX_BG 10
#define MAX_EG 10
#define MAX_CG 10
#define MAX_FW 10
#define MAX_FG 10

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

    Image(ImageManager &, std::string = "", int = 0, int = 0, Uint8 = 255);

    bool isActive() { return !baseName.empty(); }

    const Stdinfo getStdinfo();

    const std::pair<int, int> getShifts() { return {xShift, yShift}; }

    void fetch();

    void update(const std::string &, int, int);

    void render();

    void clear();

    const json dump();

    bool isCached();

    void blend(const unsigned int target) { targetAlpha = target; }

    void move(const unsigned int, const int, const int);

    void fade(const unsigned int, const Uint8, const Uint8);

    virtual void render(int, int);

protected:
    virtual void display(std::string &, const int, const int, const Uint8);

    void set(const std::string &, int, int);

    ImageManager &imageManager;

    TextureCache &textureCache;

    SDL_Renderer *renderer;

private:
    bool moving = false;
    bool transitioning = false;
    bool fading = false;

    // Info about the previous image for fading out
    std::string prevBaseName;
    Uint8 prevTargetAlpha;
    int prevXShift;
    int prevYShift;

    // Number of frames to take for movement
    unsigned int moveRdraw;

    // Framestamp of start of movement
    Uint64 moveStart;

    int targetXShift;
    int targetYShift;

    Uint8 targetAlpha;

    Uint64 fadeStart;
    Uint8 startAlpha;
    unsigned int fadeFrames;
};

class Choice : public Image
{
private:
    void renderText(const int, const int);

public:
    const std::string target;

    const std::string prompt;

    Choice(ImageManager &, const std::string &, const std::string &);

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

class Fg : public Image
{
    // Inherit ctors
    using Image::Image;

protected:
    // void display(std::string &name, const int x, const int y, const Uint8 alpha) { Image::display(name, x, y + FG_YSHIFT, alpha); }
};

class Base : public Image
{
    using Image::Image;
};

class Part1 : public Image
{
    using Image::Image;
};

class Part2 : public Image
{
    using Image::Image;
};

class Cg : public Base, public Part1, public Part2
{
public:
    const Stdinfo getStdinfo() { return Base::getStdinfo(); };

    const json dump();

    const std::pair<int, int> getShifts() { return Base::getShifts(); }

    bool isActive() { return Base::isActive(); }

    std::string rawName;

    Cg(ImageManager &);

    void clear();

    void update(const std::string &, int, int);

    void render();

    void blend(const unsigned int);

    void move(const unsigned int, const int, const int);

    void fade(const unsigned int, const Uint8, const Uint8);

private:
    bool isReady();
};

class Fw : public Cg
{
    using Cg::Cg;

protected:
    void display(std::string &name, const int x, const int y, const Uint8 alpha) { Base::display(name, x + FW_XSHIFT, y + FW_YSHIFT, alpha); }
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

    void fade(const int i, const unsigned int frames, const Uint8 start, const Uint8 end)
    {
        if (i >= size())
            throw std::runtime_error("Out of range access");
        objects[i].fade(frames, start, end);
    }

    void move(const int i, const unsigned int rdraw, const int targetXShift, const int targetYshift)
    {
        if (i >= size())
            throw std::runtime_error("Out of range access");
        objects[i].move(rdraw, targetXShift, targetYshift);
    }

    void blend(const int i, const unsigned int target)
    {
        if (i >= size())
            throw std::runtime_error("Out of range access");
        objects[i].blend(target);
    }

    const Stdinfo getStdinfo(size_t i)
    {
        if (i >= size())
            throw std::runtime_error("Out of range access");
        return objects[i].getStdinfo();
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
