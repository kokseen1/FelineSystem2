#include <imgtypes.hpp>

#include <SDL2/SDL_ttf.h>

Image::Image(SDL_Renderer *r, std::string n, int x, int y) : renderer{r}, textureName{n}, xShift{x}, yShift{y}
{
}

void Image::clear()
{
    textureName.clear();
}

Choice::Choice(SDL_Renderer *r, const std::string &t, const std::string &p) : Image(r, SEL, 0, 0), target(t), prompt(p)
{
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

    if (textureData == NULL)
    {
        // Look for texture in cache
        auto got = textureCache.find(textureName);
        if (got == textureCache.end())
            return;

        textureData = &got->second;
    }

    auto texture = textureData->first;
    if (texture == NULL)
    {
        LOG << "NULL texture in cache";
        return;
    }

    // Calculate shift
    auto &stdinfo = textureData->second;
    auto xPos = stdinfo.OffsetX - stdinfo.BaseX + xShift;
    auto yPos = stdinfo.OffsetY - stdinfo.BaseY + yShift;

    // Render onto canvas
    SDL_Rect DestR{xPos, yPos, static_cast<int>(stdinfo.Width), static_cast<int>(stdinfo.Height)};
    SDL_RenderCopyEx(renderer, texture, NULL, &DestR, 0, 0, RENDERER_FLIP_MODE);
}

// Render image with the offsets set in ctor
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
    base.render();
    part1.render();
    part2.render();
}

void Cg::clear()
{
    assetRaw.clear();

    base.clear();
    part1.clear();
    part2.clear();
}
