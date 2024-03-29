#include <image.hpp>
#include <hgdecoder.hpp>
#include <utils.hpp>
#include <sys_mwnd.h>
#include <sys_sel.h>

#include <iostream>
#include <sstream>
#include <vector>

// Clear the entire canvas
void ImageManager::clearCanvas()
{
    bgLayer.clear();
    egLayer.clear();
    cgLayer.clear();
    fwLayer.clear();
    fgLayer.clear();

    killRdraw();
}

// Load a json object of dumped image data
void ImageManager::loadDump(const json &j)
{
    clearCanvas();

    bgLayer.load(j.at(KEY_BG));
    egLayer.load(j.at(KEY_EG));
    // fgLayer.load(j.at(KEY_FG));
    cgLayer.load(j.at(KEY_CG));
    fwLayer.load(j.at(KEY_FW));
}

// Dump all images currently on the canvas to a json object
const json ImageManager::dump()
{
    return {
        {KEY_BG, bgLayer.dump()},
        {KEY_EG, egLayer.dump()},
        // {KEY_FG, fgLayer.dump()},
        {KEY_CG, cgLayer.dump()},
        {KEY_FW, fwLayer.dump()}};
}

ImageManager::ImageManager(FileManager &fm, SDL_Renderer *renderer, std::vector<Choice> &currChoices) : fileManager{fm}, renderer{renderer}, bgLayer{*this}, egLayer{*this}, cgLayer{*this}, fwLayer{*this}, fgLayer{*this}, currChoices{currChoices}
{
    // Background color when rendering transparent textures
    if (SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0) < 0)
    {
        throw std::runtime_error("Could not set render draw color");
    }

    // Blending for RenderCopy
    if (SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND) < 0)
    {
        throw std::runtime_error("Could not set render draw blend mode");
    }

    // For scaling in fullscreen
    if (SDL_RenderSetLogicalSize(renderer, WINDOW_WIDTH, WINDOW_HEIGHT) < 0)
    {
        throw std::runtime_error("Could not set logical size");
    }

    font = TTF_OpenFont(FONT_PATH, FONT_SIZE);
    if (font == NULL)
    {
        throw std::runtime_error("Could not open font");
    }

    // Decode and cache choice selection asset
    processImage(sys_sel, sizeof(sys_sel), {SEL, 2});

    // Decode and cache message window assets
    processImage(sys_mwnd, sizeof(sys_mwnd), {MWND, 43});
    processImage(sys_mwnd, sizeof(sys_mwnd), {MWND_DECO, 42});

    // LOG << "ImageManager initialized";
}

// Returns a pointer to a texture from a given frame
// Caller is responsible for freeing the texture
SDL_Texture *ImageManager::getTextureFromFrame(HGDecoder::Frame frame)
{
    auto rgbaVec = HGDecoder::getPixelsFromFrame(frame);
    if (rgbaVec.empty())
    {
        LOG << "Could not get pixels from frame";
        return NULL;
    }

    // Pixel buffer must remain alive when using surface
    SDL_Surface *surface = SDL_CreateRGBSurfaceFrom(rgbaVec.data(), frame.Stdinfo->Width, frame.Stdinfo->Height, frame.Stdinfo->BitDepth, PITCH(frame.Stdinfo->Width, frame.Stdinfo->BitDepth), RMASK, GMASK, BMASK, AMASK);

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);

    SDL_FreeSurface(surface);

    return texture;
}

// Render the current speaker name
void ImageManager::renderSpeaker(const std::string &text)
{
    if (text.empty())
    {
        return;
    }

    // Render text
    SDL_Surface *surface = TTF_RenderUTF8_Blended_Wrapped(font, ("[ " + text + " ]").c_str(), textColor, 0);
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

    // Horizontally center text
    SDL_Rect rect = {SPEAKER_XPOS, SPEAKER_YPOS, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &rect);

    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

// Render text onto the message window
void ImageManager::renderMessage(const std::string &text)
{
    if (text.empty())
    {
        return;
    }

    // Render text
    SDL_Surface *surface = TTF_RenderUTF8_Blended_Wrapped(font, text.c_str(), textColor, TEXTBOX_WIDTH);
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

    // Horizontally center text
    SDL_Rect rect = {TEXT_XPOS, TEXT_YPOS, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &rect);

    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

// Render choice textures
void ImageManager::renderChoices()
{
    auto sz = currChoices.size();
    if (sz == 0)
        return;

    auto blockHeight = sz * SEL_HEIGHT + (sz - 1) * SEL_SPACING;
    auto yShift = WINDOW_HEIGHT / 2 - blockHeight / 2;

    for (auto choice : currChoices)
    {
        choice.render(yShift);
        yShift += SEL_HEIGHT + SEL_SPACING;
    }
}

// Render images in order of type precedence and z-index
void ImageManager::render()
{
    framestamp++;

    // Clear render canvas
    SDL_RenderClear(renderer);

    bgLayer.render();
    cgLayer.render();
    egLayer.render();
    fgLayer.render();

    if (showMwnd)
    {
        // Render message window
        mwnd.render();
        mwndDeco.render();

        if (showText)
        {
            fwLayer.render();

            renderMessage(currText);
            renderSpeaker(currSpeaker);
        }
    }

    renderChoices();

    // Update screen
    SDL_RenderPresent(renderer);
}

// Clear image of type at specified z index
void ImageManager::clearZIndex(const IMAGE_TYPE type, const int zIndex)
{
    switch (type)
    {
    case IMAGE_TYPE::BG:
        bgLayer.clear(zIndex);
        break;

    case IMAGE_TYPE::EG:
        egLayer.clear(zIndex);
        break;

    case IMAGE_TYPE::FG:
        fgLayer.clear(zIndex);
        break;

    case IMAGE_TYPE::CG:
        cgLayer.clear(zIndex);
        break;

    case IMAGE_TYPE::FW:
        fwLayer.clear(zIndex);
        break;
    }
}

// Clear all layers of specified image type
void ImageManager::clearImageType(const IMAGE_TYPE type)
{
    switch (type)
    {
    case IMAGE_TYPE::BG:
        bgLayer.clear();
        break;

    case IMAGE_TYPE::EG:
        egLayer.clear();
        break;

    case IMAGE_TYPE::FG:
        fgLayer.clear();
        break;

    case IMAGE_TYPE::CG:
        cgLayer.clear();
        break;

    case IMAGE_TYPE::FW:
        fwLayer.clear();
        break;
    }
}

const Stdinfo ImageManager::getStdinfo(const IMAGE_TYPE type, const int zIndex)
{
    switch (type)
    {
    case IMAGE_TYPE::BG:
        return bgLayer.getStdinfo(zIndex);

    case IMAGE_TYPE::EG:
        return egLayer.getStdinfo(zIndex);

    case IMAGE_TYPE::FG:
        return fgLayer.getStdinfo(zIndex);

    case IMAGE_TYPE::CG:
        return cgLayer.getStdinfo(zIndex);

    case IMAGE_TYPE::FW:
        return fwLayer.getStdinfo(zIndex);
    }
}

// Get reference to specified image type at z-index
// Reference to base image for CG/FW
const std::pair<int, int> ImageManager::getShifts(const IMAGE_TYPE type, const int zIndex)
{
    switch (type)
    {
    case IMAGE_TYPE::BG:
        return bgLayer.getShifts(zIndex);

    case IMAGE_TYPE::EG:
        return egLayer.getShifts(zIndex);

    case IMAGE_TYPE::FG:
        return fgLayer.getShifts(zIndex);

    // Assume that any valid CG/FW must always contain a valid base
    case IMAGE_TYPE::CG:
        return cgLayer.getShifts(zIndex);

    case IMAGE_TYPE::FW:
        return fwLayer.getShifts(zIndex);
    }
}

void ImageManager::setFade(const IMAGE_TYPE type, const int zIndex, const unsigned int frames, const Uint8 start, const Uint8 end)
{
    switch (type)
    {
    case IMAGE_TYPE::BG:
        bgLayer.fade(zIndex, frames, start, end);
        break;

    case IMAGE_TYPE::EG:
        egLayer.fade(zIndex, frames, start, end);
        break;

    case IMAGE_TYPE::FG:
        fgLayer.fade(zIndex, frames, start, end);
        break;

    case IMAGE_TYPE::CG:
        cgLayer.fade(zIndex, frames, start, end);
        break;

    case IMAGE_TYPE::FW:
        fwLayer.fade(zIndex, frames, start, end);
        break;
    }
}

void ImageManager::setMove(const IMAGE_TYPE type, const int zIndex, const unsigned int rdraw, const int x, const int y)
{
    switch (type)
    {
    case IMAGE_TYPE::BG:
        bgLayer.move(zIndex, rdraw, x, y);
        break;

    case IMAGE_TYPE::EG:
        egLayer.move(zIndex, rdraw, x, y);
        break;

    case IMAGE_TYPE::FG:
        fgLayer.move(zIndex, rdraw, x, y);
        break;

    case IMAGE_TYPE::CG:
        cgLayer.move(zIndex, rdraw, x, y);
        break;

    case IMAGE_TYPE::FW:
        fwLayer.move(zIndex, rdraw, x, y);
        break;
    }
}

void ImageManager::setBlend(const IMAGE_TYPE type, const int zIndex, const unsigned int alpha)
{
    switch (type)
    {
    case IMAGE_TYPE::BG:
        bgLayer.blend(zIndex, alpha);
        break;

    case IMAGE_TYPE::EG:
        egLayer.blend(zIndex, alpha);
        break;

    case IMAGE_TYPE::FG:
        fgLayer.blend(zIndex, alpha);
        break;

    case IMAGE_TYPE::CG:
        cgLayer.blend(zIndex, alpha);
        break;

    case IMAGE_TYPE::FW:
        fwLayer.blend(zIndex, alpha);
        break;
    }
}

void ImageManager::setImage(const IMAGE_TYPE type, const int zIndex, std::string rawName, int xShift, int yShift)
{
    switch (type)
    {
    case IMAGE_TYPE::BG:
        bgLayer.update(zIndex, rawName, xShift, yShift);
        break;

    case IMAGE_TYPE::EG:
        egLayer.update(zIndex, rawName, xShift, yShift);
        break;

    case IMAGE_TYPE::FG:
        fgLayer.update(zIndex, rawName, xShift, yShift);
        break;

    case IMAGE_TYPE::CG:
        cgLayer.update(zIndex, rawName, xShift, yShift);
        break;

    case IMAGE_TYPE::FW:
        fwLayer.update(zIndex, rawName, xShift, yShift);
        break;
    }
}

// Called when image has been fetched
// Will return early and skip processing in async fetch if image was already passed
void ImageManager::processImage(byte *buf, size_t sz, const ImageData &imageData)
{
    const auto &name = imageData.name;
    const auto &frameIdx = imageData.index;
    const Image *image = imageData.image;

    // Do not process if image was already passed
    if (image != NULL && image->baseName != name)
        return;

    // Decode a raw HG buffer and cache the texture
    HGHeader *hgHeader = reinterpret_cast<HGHeader *>(buf);

    // Verify signature
    if (strncmp(hgHeader->FileSignature, IMAGE_SIGNATURE, sizeof(hgHeader->FileSignature)) != 0)
    {
        LOG << "Invalid image file signature for " << name;
        return;
    }

    // Retrieve frames
    FrameHeader *frameHeader = reinterpret_cast<FrameHeader *>(hgHeader + 1);
    const auto &frames = HGDecoder::getFrames(frameHeader);
    if (frames.empty())
    {
        LOG << "No frames found";
        return;
    }

    if (frames.size() > 1)
    {
        LOG << name << " contains " << frames.size() << " frames; Size: " << sz;
    }

    auto &frame = frames[frameIdx];

    SDL_Texture *texture = getTextureFromFrame(frame);

    // Store texture in cache
    textureCache[name] = {texture, *frame.Stdinfo};

    LOG << "Cached: " << name << "[" << frameIdx << "]";
}

// Forcefully complete any transition/animation
void ImageManager::killRdraw()
{
    setRdraw(0);
    render();
}

// Sets the number of frames to take to render any upcoming transition/animation
// Setting rdraw to 0 instantly displays the resulting appearance
void ImageManager::setRdraw(const unsigned int rdraw)
{
    rdrawStart = getFramestamp();
    globalRdraw = rdraw;
}

void ImageManager::setShowMwnd()
{
    mwnd.blend(MWND_ALPHA);
    mwndDeco.blend(MAX_ALPHA);
    showMwnd = true;
}

void ImageManager::setHideMwnd()
{
    showMwnd = false;
}

void ImageManager::setFrameon(const unsigned int frames)
{
    mwnd.fade(frames, 0, MWND_ALPHA);
    mwndDeco.fade(frames, 0, MAX_ALPHA);
}

void ImageManager::setFrameoff(const unsigned int frames)
{
    mwnd.fade(frames, MWND_ALPHA, 0);
    mwndDeco.fade(frames, MAX_ALPHA, 0);
}

bool ImageManager::isCached(const std::string &name)
{
    auto pos = textureCache.find(name);
    if (pos != textureCache.end())
        return true;

    return false;
}

// Create a new solid rectangle texture and cache it
void ImageManager::createSolid(const std::string &name, const int width, const int height, Uint32 color)
{
    if (isCached(name))
        return;

    SDL_Surface *surface = SDL_CreateRGBSurface(0, width, height, 32, 0xFF0000, 0xFF00, 0xFF, 0xFF000000);
    SDL_FillRect(surface, NULL, color);

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    Stdinfo stdinfo = {static_cast<uint32>(width), static_cast<uint32>(height)};

    textureCache[name] = {texture, stdinfo};

    SDL_FreeSurface(surface);
}

void ImageManager::fetch(const std::string &baseName)
{
    if (isCached(baseName))
        return;

    getFileManager().fetchAssetAndProcess(baseName + IMAGE_EXT, this, &ImageManager::processImage, ImageData{baseName, 0, NULL});
}

void ImageManager::prefetch(const std::string &asset)
{
    fetch(asset);

    const auto cgArgs = Cg::getCgArgs(asset);
    if (cgArgs.size() != 3)
        return;

    fetch(cgArgs[0]);
    fetch(cgArgs[1]);
    fetch(cgArgs[2]);
}
