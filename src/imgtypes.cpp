#include <image.hpp>
#include <imgtypes.hpp>

#include <SDL2/SDL_ttf.h>

// Constructor for non-updating images
Image::Image(ImageManager &imageManager, std::string n, int x, int y, Uint8 alpha) : imageManager{imageManager}, renderer{imageManager.getRenderer()}, textureCache{imageManager.getCache()}, baseName{n}, xShift{x}, yShift{y}, targetAlpha{alpha} {}

// Main constructor to init with renderer and cache (not required as default arguments are available for the above constructor)
// Image::Image(ImageManager &imageManager) : imageManager{imageManager}, renderer{imageManager.getRenderer()}, textureCache{imageManager.getCache()} {}

// Constructor to init both inherited base and additional parts
Cg::Cg(ImageManager &imageManager) : Base{imageManager}, Part1{imageManager}, Part2{imageManager} {}

// Custom ctor for choices
Choice::Choice(ImageManager &imageManager, const std::string &t, const std::string &p) : Image{imageManager, SEL, SEL_XSHIFT, 0}, target{t}, prompt{p} {}

void Image::update(const std::string &name, int x, int y)
{
    // Ensure that the asset exists in db
    if (textureCache.find(name) == textureCache.end() && !imageManager.getFileManager().inDB(name + IMAGE_EXT))
    {
        // clear();
        return;
    }

    set(name, x, y);
    fetch();
}

// Clear by setting the current name to blank to simulate a transition
void Image::clear()
{
    set("", xShift, yShift);

    // Reset target alpha
    blend(255);
}

void Image::set(const std::string &name, int x, int y)
{
    if (name != baseName)
        transitioning = true;

    // Save information about the previous image when there is a transition
    prevBaseName = baseName;
    prevTargetAlpha = targetAlpha;
    prevXShift = xShift;
    prevYShift = yShift;

    moving = false;

    baseName = name;
    xShift = x;
    yShift = y;
}

bool Image::isCached()
{
    return imageManager.isCached(baseName);
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

void Image::fade(const unsigned int frames, const Uint8 start, const Uint8 end)
{
    fadeFrames = frames;
    fadeStart = imageManager.getFramestamp();
    startAlpha = start;
    targetAlpha = end;
    fading = true;
}

void Image::move(const unsigned int rdraw, const int x, const int y)
{
    moveRdraw = rdraw;
    moveStart = imageManager.getFramestamp();
    targetXShift = x;
    targetYShift = y;
    moving = true;
}

// Internal function to render the image
void Image::display(std::string &name, long xPos, long yPos, const Uint8 alpha, bool absolute)
{
    if (name.empty())
        return;

    // Look for texture in cache
    auto got = textureCache.find(name);
    if (got == textureCache.end())
    {
        LOG << "Cannot find in cache " << name;
        return;
    }

    const auto &textureData = got->second;

    auto texture = textureData.first;
    if (texture == NULL)
    {
        LOG << "NULL texture in cache";
        return;
    }

    SDL_SetTextureAlphaMod(texture, alpha);

    auto &stdinfo = textureData.second;

    if (!absolute)
    {
        // Calculate shift
        xPos = stdinfo.OffsetX - stdinfo.BaseX + xPos;
        yPos = stdinfo.OffsetY - stdinfo.BaseY + yPos;
    }

    // LOG << baseName << " " << stdinfo.OffsetX << " " << stdinfo.BaseX << " " << " " << stdinfo.Width << " " << xPos;
    // LOG << baseName << " " << stdinfo.OffsetY << " " << stdinfo.BaseY << " " << " " << stdinfo.Height << " " << yPos;

    // Render onto canvas
    SDL_Rect DestR{xPos, yPos, static_cast<int>(stdinfo.Width), static_cast<int>(stdinfo.Height)};
    SDL_RenderCopyEx(renderer, texture, NULL, &DestR, 0, 0, RENDERER_FLIP_MODE);
}

const Stdinfo Image::getStdinfo()
{
    if (baseName.empty())
        return {};

    auto got = textureCache.find(baseName);
    if (got == textureCache.end())
        return {};

    const auto &textureData = got->second;

    return textureData.second;
}

double easeInOutQuad(double x)
{
    return x < 0.5 ? 2 * x * x : 1 - pow(-2 * x + 2, 2) / 2;
}

// Render image with given offsets
void Image::render(int x, int y)
{
    Uint8 alpha = targetAlpha;
    Uint8 prevAlphaInverse = prevTargetAlpha;

    // Only fade if rdraw is a valid value
    // Otherwise, set to target alpha values directly from above
    if (transitioning && imageManager.getGlobalRdraw() > 0)
    {
        double ratio = (double)(imageManager.getFramestamp() - imageManager.getRdrawStart()) / imageManager.getGlobalRdraw();

        alpha = easeInOutQuad(ratio) * targetAlpha;
        prevAlphaInverse = easeInOutQuad(ratio) * prevTargetAlpha;
        // LOG << baseName << " : " << (int)alpha << ", " << prevBaseName << " : " << (int) prevTargetAlpha - prevAlphaInverse << " p"<< (int)prevTargetAlpha;
    }

    // Fade overrides any transitions
    if (fading && fadeFrames > 0)
    {
        double ratio = (double)(imageManager.getFramestamp() - fadeStart) / fadeFrames;

        alpha = startAlpha + (targetAlpha - startAlpha) * easeInOutQuad(ratio);
        // LOG << baseName << " : " << (int)alpha << " t" << (int)targetAlpha;
    }

    if (moving && moveRdraw > 0)
    {
        double ratio = (double)(imageManager.getFramestamp() - moveStart) / moveRdraw;

        x = (double)(targetXShift - xShift) * easeInOutQuad(ratio) + xShift;
        y = (double)(targetYShift - yShift) * easeInOutQuad(ratio) + yShift;
        // LOG << baseName << " : " << nextX << ", " << nextY << ", " << moveRdraw;
    }

    if (x == targetXShift && y == targetYShift)
    {
        xShift = targetXShift;
        yShift = targetYShift;
        moving = false;
    }

    if (alpha == targetAlpha)
    {
        transitioning = false;
        fading = false;
    }

    // LOG << baseName << prevTargetAlpha << prevBaseName << prevAlphaInverse;
    display(prevBaseName, prevXShift, prevYShift, prevTargetAlpha - prevAlphaInverse);
    display(baseName, x, y, alpha);
}

// Render image with the member offsets by default
void Image::render()
{
    render(xShift, yShift);
}

const json Image::dump()
{
    return {
        {KEY_NAME, baseName},
        {KEY_XSHIFT, xShift},
        {KEY_YSHIFT, yShift}};
}

// Dump raw name and use Base offsets
const json Cg::dump()
{
    return {
        {KEY_NAME, rawName},
        {KEY_XSHIFT, Base::xShift},
        {KEY_YSHIFT, Base::yShift}};
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
    // Will only render the sprite when all sub-sprites are ready
    // if (!isReady())
    //     return;

    Base::render();
    Part1::render();
    Part2::render();
}

void Cg::clear()
{
    rawName.clear();

    Base::clear();
    Part1::clear();
    Part2::clear();
}

void Cg::fade(const unsigned int frames, const Uint8 start, const Uint8 end)
{
    Base::fade(frames, start, end);
    Part1::fade(frames, start, end);
    Part2::fade(frames, start, end);
}

void Cg::move(const unsigned int rdraw, const int x, const int y)
{
    Base::move(rdraw, x, y);
    Part1::move(rdraw, x, y);
    Part2::move(rdraw, x, y);
}

void Cg::blend(const unsigned int target)
{
    Base::blend(target);
    Part1::blend(target);
    Part2::blend(target);
}

// Return if the multi-part sprite is cached and ready to be rendered as a whole
bool Cg::isReady()
{
    if (Base::isActive() && !Base::isCached())
        return false;

    if (Part1::isActive() && !Part1::isCached())
        return false;

    if (Part2::isActive() && !Part2::isCached())
        return false;

    return true;
}

void Cg::update(const std::string &rawName, int x, int y)
{
    const auto &args = Utils::getAssetArgs(rawName);
    // TODO: Handle arguments like `blend`
    if (args.size() < 5)
        return;

    Cg::rawName = rawName;

    const std::string &rawBase = args[0];
    const std::string &baseName = rawBase + "_" + args[1];
    const std::string &part1Name = rawBase + "_" + Utils::zeroPad(args[3], 3);
    const std::string &part2Name = rawBase + "_" + Utils::zeroPad(args[4], 4);

    Base::update(baseName, x, y);
    Part1::update(part1Name, x, y);
    Part2::update(part2Name, x, y);
}
