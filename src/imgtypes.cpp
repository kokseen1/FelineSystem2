#include <image.hpp>
#include <imgtypes.hpp>

#include <SDL2/SDL_ttf.h>

// Constructor for non-updating images
Image::Image(ImageManager &imageManager, std::string n, int x, int y) : imageManager{imageManager}, renderer{imageManager.getRenderer()}, textureCache{imageManager.getCache()}, baseName{n}, xShift{x}, yShift{y} {}

// Main constructor to init with renderer and cache (not required as default arguments are available for the above constructor)
// Image::Image(ImageManager &imageManager) : imageManager{imageManager}, renderer{imageManager.getRenderer()}, textureCache{imageManager.getCache()} {}

// Constructor to init both inherited base and additional parts
Cg::Cg(ImageManager &imageManager) : Image{imageManager}, part1{imageManager}, part2{imageManager} {}

// Custom ctor for choices
Choice::Choice(ImageManager &imageManager, const std::string &t, const std::string &p) : Image{imageManager, SEL, SEL_XSHIFT, 0}, target{t}, prompt{p} {}

void Image::update(const std::string &name, int x, int y)
{
    // Ensure that the asset exists in db
    if (!imageManager.getFileManager().inDB(name + IMAGE_EXT))
        return;

    set(name, x, y);
    fetch();
}

void Image::set(const std::string &name, int x, int y)
{
    baseName = name;
    xShift = x;
    yShift = y;
}

bool Image::isCached()
{
    auto pos = textureCache.find(baseName);
    if (pos != textureCache.end())
        return true;

    return false;
}

// Fetch and cache the current image
void Image::fetch()
{
    // Prevent fetching already cached images
    if (isCached())
        return;

    // Initialize cache entry with NULL (more efficient but prevents failed fetches from retrying)
    // textureCache[name];

    imageManager.getFileManager().fetchAssetAndProcess(baseName + IMAGE_EXT, &imageManager, &ImageManager::processImage, ImageData{baseName, 0, this});
}

void Choice::render(const int y)
{
    // Render base box image
    Image::render(xShift, y);
    renderText(xShift, y);
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

const json Image::dump()
{
    return {
        {KEY_NAME, getRawName()},
        {KEY_XSHIFT, xShift},
        {KEY_YSHIFT, yShift}};
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

// Private render implementation
// Will only render the sprite when all sub-sprites are ready
void Cg::render(int x, int y)
{
    if (!isReady())
        return;

    Image::render(x, y);
    part1.render(x, y);
    part2.render(x, y);
}

void Cg::render()
{
    render(xShift, yShift);
}

void Cg::clear()
{
    rawName.clear();

    Image::clear();
    part1.clear();
    part2.clear();
}

// Return if the multi-part sprite is cached and ready to be rendered as a whole
bool Cg::isReady()
{
    if (isActive() && !isCached())
        return false;

    if (part1.isActive() && !part1.isCached())
        return false;

    if (part2.isActive() && !part2.isCached())
        return false;

    return true;
}

void Cg::update(const std::string &rawName, int x, int y)
{
    const auto &args = Utils::getAssetArgs(rawName);
    // TODO: Handle arguments like `blend`
    if (args.size() < 5)
        return;

    clear();
    Cg::rawName = rawName;

    const std::string &rawBase = args[0];
    const std::string &baseName = rawBase + "_" + args[1];
    const std::string &part1Name = rawBase + "_" + Utils::zeroPad(args[3], 3);
    const std::string &part2Name = rawBase + "_" + Utils::zeroPad(args[4], 4);

    Image::update(baseName, x, y);
    part1.update(part1Name, x, y);
    part2.update(part2Name, x, y);
}

// Render at a special shifted offset for FW sprites
void Fw::render()
{
    Cg::render(xShift + FW_XSHIFT, yShift + FW_YSHIFT);
}