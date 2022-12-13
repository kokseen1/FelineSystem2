#include <image.hpp>
#include <imgtypes.hpp>

#include <SDL2/SDL_ttf.h>

// Constructor for non-updating images
Image::Image(ImageManager &imageManager, std::string n, int x, int y) : imageManager{imageManager}, renderer{imageManager.getRenderer()}, textureCache{imageManager.getCache()}, baseName{n}, xShift{x}, yShift{y} {}

// // Main constructor to init with renderer and cache
// Image::Image(ImageManager &imageManager) : imageManager{imageManager}, renderer{imageManager.getRenderer()}, textureCache{imageManager.getCache()} {}

// Constructor  o init both inherited base and additional parts
Cg::Cg(ImageManager &imageManager) : Image{imageManager}, part1{imageManager}, part2{imageManager} {}

Choice::Choice(ImageManager &imageManager, const std::string &t, const std::string &p) : Image{imageManager, SEL, 0, 0}, target{t}, prompt{p} {}

void Image::set(const std::string &name, int x, int y)
{
    baseName = name;
    xShift = x;
    yShift = y;
}

void Image::clear()
{
    baseName.clear();
}

void Choice::render(const int yShift)
{
    // Render base box image
    Image::render(SEL_XSHIFT, yShift);
    renderText(SEL_XSHIFT, yShift);
}

// Render image with given offsets
void Image::render(const int xShift, const int yShift)
{
    if (!isActive())
        return;

    // Look for texture in cache
    auto got = textureCache.find(baseName);
    if (got == textureCache.end())
        return;

    const auto &textureData = got->second;

    auto texture = textureData.first;
    if (texture == NULL)
    {
        LOG << "NULL texture in cache";
        return;
    }

    // Calculate shift
    auto &stdinfo = textureData.second;
    auto xPos = stdinfo.OffsetX - stdinfo.BaseX + xShift;
    auto yPos = stdinfo.OffsetY - stdinfo.BaseY + yShift;

    // Render onto canvas
    SDL_Rect DestR{xPos, yPos, static_cast<int>(stdinfo.Width), static_cast<int>(stdinfo.Height)};
    SDL_RenderCopyEx(renderer, texture, NULL, &DestR, 0, 0, RENDERER_FLIP_MODE);
}

// Render image with the member offsets by default
void Image::render()
{
    render(xShift, yShift);
}

// Render text for a selection box
void Choice::renderText(const int xShift, const int yShift)
{
    static TTF_Font *selectFont = TTF_OpenFont(SELECT_FONT_PATH, SELECT_FONT_SIZE);

    if (prompt.empty())
        return;

    // Render text
    SDL_Surface *surface = TTF_RenderUTF8_Blended_Wrapped(selectFont, prompt.c_str(), {0, 0, 0, 255}, 0);
    if (surface == NULL)
    {
        LOG << "TTF Surface failed!";
        return;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture == NULL)
    {
        LOG << "TTF texture failed!";
        SDL_FreeSurface(surface);
        return;
    }

    // Center text in select box
    SDL_Rect rect = {xShift + (SEL_WIDTH / 2 - surface->w / 2), yShift + (SEL_HEIGHT / 2 - surface->h / 2), surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &rect);

    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

void Cg::render()
{
    Image::render();
    part1.render();
    part2.render();
}

void Cg::clear()
{
    assetRaw.clear();

    Image::clear();
    part1.clear();
    part2.clear();
}

void Fw::render()
{
    auto x = xShift + FW_XSHIFT;
    auto y = yShift + FW_YSHIFT;

    Image::render(x, y);
    part1.render(x, y);
    part2.render(x, y);
}