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
    clearImageType(IMAGE_TYPE::BG);
    clearImageType(IMAGE_TYPE::EG);
    clearImageType(IMAGE_TYPE::CG);
    clearImageType(IMAGE_TYPE::FW);
}

// void to_json(json &j, const Image &i)
// {
//     j = json{
//         {KEY_NAME, i.baseName},
//         {KEY_XSHIFT, i.xShift},
//         {KEY_YSHIFT, i.yShift}};
// }

// void from_json(const json &j, Image &i)
// {
//     j.at(KEY_NAME).get_to(i.baseName);
//     j.at(KEY_XSHIFT).get_to(i.xShift);
//     j.at(KEY_YSHIFT).get_to(i.yShift);
// }

// void to_json(json &j, const Fw &i)
// {
//     j = json{
//         {KEY_NAME, i.assetRaw},
//         // Assume base is always valid in a valid CG/FW
//         // Undo hardcoded offset for FW
//         {KEY_XSHIFT, i.base.xShift - FW_XSHIFT},
//         {KEY_YSHIFT, i.base.yShift - FW_YSHIFT}};
// }

// void to_json(json &j, const Cg &i)
// {
//     j = json{
//         {KEY_NAME, i.assetRaw},
//         // Assume base is always valid in a valid CG
//         {KEY_XSHIFT, i.base.xShift},
//         {KEY_YSHIFT, i.base.yShift}};
// }

// void from_json(const json &j, Cg &i)
// {
//     j.at(KEY_NAME).get_to(i.assetRaw);
//     // Assume base is always valid in a valid CG
//     j.at(KEY_XSHIFT).get_to(i.base.xShift);
//     j.at(KEY_YSHIFT).get_to(i.base.yShift);
// }

// const json Image::dump()
// {
//     auto &j = *this;
//     return j;
// }

// const json Fw::dump()
// {
//     auto &j = *this;
//     return j;
// }

// const json Cg::dump()
// {
//     auto &j = *this;
//     return j;
// }

// Load a json object of dumped image data
// Throws json::out_of_range if key is missing
void ImageManager::loadDump(const json &j)
{
    clearCanvas();

    // if (j.contains(KEY_BG))
    // {
    //     for (auto &e : j[KEY_BG].items())
    //     {
    //         const auto &idx = std::stoi(e.key());
    //         auto bg = e.value().get<Bg>();
    //         setImage(IMAGE_TYPE::BG, idx, bg.baseName, bg.xShift, bg.yShift);
    //     }
    // }

    // if (j.contains(KEY_EG))
    // {
    //     for (auto &e : j[KEY_EG].items())
    //     {
    //         const auto &idx = std::stoi(e.key());
    //         auto eg = e.value().get<Eg>();
    //         setImage(IMAGE_TYPE::EG, idx, eg.baseName, eg.xShift, eg.yShift);
    //     }
    // }

    // if (j.contains(KEY_CG))
    // {
    //     for (auto &e : j[KEY_CG].items())
    //     {
    //         const auto &idx = std::stoi(e.key());
    //         auto cg = e.value().get<Cg>();
    //         setImage(IMAGE_TYPE::CG, idx, cg.assetRaw, cg.base.xShift, cg.base.yShift);
    //     }
    // }

    // if (j.contains(KEY_FW))
    // {
    //     for (auto &e : j[KEY_FW].items())
    //     {
    //         const auto &idx = std::stoi(e.key());
    //         auto fw = e.value().get<Fw>();
    //         setImage(IMAGE_TYPE::FW, idx, fw.assetRaw, fw.base.xShift, fw.base.yShift);
    //     }
    // }
}

// Dump all images currently on the canvas to a json object
const json ImageManager::dump()
{
    json j;
    //     j[KEY_BG] = bgLayer.dump();
    //     j[KEY_EG] = egLayer.dump();
    //     j[KEY_CG] = cgLayer.dump();
    //     j[KEY_FW] = fwLayer.dump();
    return j;
}

ImageManager::ImageManager(FileManager &fm, SDL_Renderer *renderer) : fileManager{fm}, renderer{renderer}, bgLayer{*this}, egLayer{*this}, cgLayer{*this}, fwLayer{*this}
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
    if (textureCache[MWND].first != NULL)
        SDL_SetTextureAlphaMod(textureCache[MWND].first, MWND_ALPHA);

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
    auto &choices = sceneManager->getCurrChoices();
    auto sz = choices.size();
    if (sz == 0)
        return;

    auto blockHeight = sz * SEL_HEIGHT + (sz - 1) * SEL_SPACING;
    auto yShift = WINDOW_HEIGHT / 2 - blockHeight / 2;

    for (auto choice : choices)
    {
        choice.render();
        yShift += SEL_HEIGHT + SEL_SPACING;
    }
}

// Render images in order of type precedence and z-index
void ImageManager::render()
{
    // Clear render canvas
    SDL_RenderClear(renderer);

    bgLayer.render();
    cgLayer.render();
    egLayer.render();

    if (showMwnd)
    {
        // Render message window
        mwnd.render();
        mwndDeco.render();

        fwLayer.render();

        renderMessage(currText);
        renderSpeaker(currSpeaker);
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

    case IMAGE_TYPE::CG:
        cgLayer.clear();
        break;

    case IMAGE_TYPE::FW:
        fwLayer.clear();
        break;
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

    // Assume that any valid CG/FW must always contain a valid base
    case IMAGE_TYPE::CG:
        return cgLayer.getShifts(zIndex);

    case IMAGE_TYPE::FW:
        return fwLayer.getShifts(zIndex);
    }
}

// Names of assets must be inserted in ascending z-index
void ImageManager::setImage(const IMAGE_TYPE type, const int zIndex, std::string rawName, int xShift, int yShift)
{

#ifdef LOWERCASE_ASSETS
    // Convert names to lowercase
    Utils::lowercase(rawName);
#endif

    switch (type)
    {

    case IMAGE_TYPE::BG:
        bgLayer.update(zIndex, rawName, xShift, yShift);
        break;
    case IMAGE_TYPE::EG:
        egLayer.update(zIndex, rawName, xShift, yShift);
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
    textureCache[name] = std::make_pair(texture, *frame.Stdinfo);

    LOG << "Cached: " << name << "[" << frameIdx << "]";
}
