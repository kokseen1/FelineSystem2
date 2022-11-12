#include <image.hpp>
#include <hgdecoder.hpp>
#include <utils.hpp>
#include <sys_mwnd.h>

#include <iostream>
#include <sstream>
#include <vector>

static std::map<std::string, TextureData> textureCache;

Image::Image(std::string n, int x, int y) : textureName{n}, xShift{x}, yShift{y}
{
}

void Image::clear()
{
    textureName.clear();
}

// Render image onto the given renderer
void Image::render(SDL_Renderer *renderer)
{
    if (textureName.empty())
        // Image is inactive
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

void Cg::render(SDL_Renderer *renderer)
{
    base.render(renderer);
    part1.render(renderer);
    part2.render(renderer);
}

void Cg::clear()
{
    base.clear();
    part1.clear();
    part2.clear();
}

ImageManager::ImageManager(FileManager *fm) : fileManager{fm}
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        throw std::runtime_error("Could not initialize SDL");
    }

    if (TTF_Init() == -1)
    {
        throw std::runtime_error("Could not init TTF");
    }

    // Create SDL window and renderer
    window = SDL_CreateWindow("FelineSystem2",
                              SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              WINDOW_WIDTH,
                              WINDOW_HEIGHT,
                              SDL_WINDOW_SHOWN);
    if (window == NULL)
    {
        throw std::runtime_error("Could not create SDL window");
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == NULL)
    {
        throw std::runtime_error("Could not create SDL renderer");
    }

    setWindowIcon(window);

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

    // Decode and cache message window assets
    processImage(sys_mwnd, sizeof(sys_mwnd), std::pair<std::string, int>(MWND, 43));
    processImage(sys_mwnd, sizeof(sys_mwnd), std::pair<std::string, int>(MWND_DECO, 42));
    if (textureCache[MWND].first != NULL)
        SDL_SetTextureAlphaMod(textureCache[MWND].first, 120);

    LOG << "ImageManager initialized";
}

void ImageManager::toggle_fullscreen()
{
    // #ifdef __EMSCRIPTEN__
    //     EmscriptenFullscreenChangeEvent fsce;
    //     EMSCRIPTEN_RESULT ret = emscripten_get_fullscreen_status(&fsce);
    //     if (!fsce.isFullscreen)
    //     {
    //         printf("Requesting fullscreen..\n");
    //         ret = emscripten_request_fullscreen("#canvas", 1);
    //     }
    //     else
    //     {
    //         printf("Exiting fullscreen..\n");
    //         ret = emscripten_exit_fullscreen();
    //         ret = emscripten_get_fullscreen_status(&fsce);
    //         if (fsce.isFullscreen)
    //         {
    //             fprintf(stderr, "Fullscreen exit did not work!\n");
    //         }
    //     }
    // #else
    // SDL fullscreen
    static int isFullscreen = 0;
    switch (isFullscreen)
    {
    case 0:
        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
        break;
    case 1:
        SDL_SetWindowFullscreen(window, 0);
        break;
    }
    isFullscreen ^= 1;
    // #endif
}

// Set the SDL window icon using an array of pixels
void ImageManager::setWindowIcon(SDL_Window *window)
{
#include <logo.h>
    SDL_Surface *surface = SDL_CreateRGBSurfaceFrom(logo, 64, 64, 32, 64 * 4, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
    if (surface == NULL)
    {
        LOG << "Could not create surface from logo";
        return;
    }

    SDL_SetWindowIcon(window, surface);
    SDL_FreeSurface(surface);
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
    SDL_Surface *surface = TTF_RenderUTF8_Blended_Wrapped(font, text.c_str(), textColor, 0);
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

// Render images in order of type precedence and z-index
void ImageManager::render()
{
    // Clear render canvas
    SDL_RenderClear(renderer);

    for (auto &bg : currBgs)
        bg.render(renderer);

    for (auto &cg : currCgs)
        cg.render(renderer);

    for (auto &eg : currEgs)
        eg.render(renderer);

    // Render message window
    mwnd.render(renderer);
    mwndDeco.render(renderer);

    for (auto &fw : currFws)
        fw.render(renderer);

    renderMessage(currText);
    renderSpeaker(currSpeaker);

    // Update screen
    SDL_RenderPresent(renderer);
}

// Clear image of type at specified z index
void ImageManager::clearZIndex(IMAGE_TYPE type, int zIndex)
{
    switch (type)
    {
    case IMAGE_TYPE::BG:
        if (zIndex >= currBgs.size())
            return;
        currBgs[zIndex].clear();
        break;

    case IMAGE_TYPE::EG:
        if (zIndex >= currEgs.size())
            return;
        currEgs[zIndex].clear();
        break;

    case IMAGE_TYPE::CG:
        if (zIndex >= currCgs.size())
            return;
        currCgs[zIndex].clear();
        break;

    case IMAGE_TYPE::FW:
        if (zIndex >= currFws.size())
            return;
        currFws[zIndex].clear();
        break;

    default:
        break;
    }
}

// Clear all layers of specified image type
void ImageManager::clearImageType(IMAGE_TYPE type)
{
    switch (type)
    {
    case IMAGE_TYPE::BG:
        for (int i = 0; i < currBgs.size(); i++)
            currBgs[i].clear();
        break;

    case IMAGE_TYPE::EG:
        for (int i = 0; i < currEgs.size(); i++)
            currEgs[i].clear();
        break;

    case IMAGE_TYPE::CG:
        for (int i = 0; i < currCgs.size(); i++)
            currCgs[i].clear();
        break;

    case IMAGE_TYPE::FW:
        for (int i = 0; i < currFws.size(); i++)
            currFws[i].clear();
        break;

    default:
        break;
    }
}

// Get ImageData of specified image type at z-index
ImageData ImageManager::getImageData(IMAGE_TYPE type, int zIndex)
{
    switch (type)
    {
    case IMAGE_TYPE::BG:
        return ImageData{};
    case IMAGE_TYPE::EG:
        return ImageData{};
    case IMAGE_TYPE::CG:
        return ImageData{};
    case IMAGE_TYPE::FW:
        return ImageData{};
    }
}

// Parse comma separated asset names
std::vector<std::string> ImageManager::getAssetArgs(const std::string &asset)
{
    std::stringstream ss(asset);
    std::vector<std::string> args;

    while (ss.good())
    {
        std::string arg;
        getline(ss, arg, ',');
        args.push_back(arg);
    }

    return args;
}
// Parses image arguments into ImageData to be displayed
// Names of assets must be inserted in ascending z-index
void ImageManager::setImage(IMAGE_TYPE type, int zIndex, std::string asset, int xShift, int yShift)
{

#ifdef LOWERCASE_ASSETS
    // Convert names to lowercase
    Utils::lowercase(asset);
#endif

    std::vector<std::string> assetsToFetch;

    switch (type)
    {

    case IMAGE_TYPE::BG:
        if (zIndex >= currBgs.size())
            return;
        if (!fileManager->inDB(asset + IMAGE_EXT))
            return;

        currBgs[zIndex] = {asset, xShift, yShift};
        assetsToFetch.push_back(asset);
        break;

    case IMAGE_TYPE::EG:
        if (zIndex >= currEgs.size())
            return;
        if (!fileManager->inDB(asset + IMAGE_EXT))
            return;

        currEgs[zIndex] = {asset, xShift, yShift};
        assetsToFetch.push_back(asset);
        break;

    case IMAGE_TYPE::CG:
    {
        if (zIndex >= currCgs.size())
            return;

        auto args = getAssetArgs(asset);
        if (args.size() < 5)
        {
            LOG << "Invalid CG args for '" << asset << "'";
            return;
        }

        const std::string &baseRaw = args[0];
        const std::string &baseName = baseRaw + "_" + args[1];
        const std::string &part1Name = baseRaw + "_" + Utils::zeroPad(args[3], 3);
        const std::string &part2Name = baseRaw + "_" + Utils::zeroPad(args[4], 4);

        Cg &cg = currCgs[zIndex];
        cg.clear();

        if (fileManager->inDB(baseName + IMAGE_EXT))
        {
            cg.base = Image{baseName, xShift, yShift};
            assetsToFetch.push_back(baseName);
        }

        if (fileManager->inDB(part1Name + IMAGE_EXT))
        {
            cg.part1 = Image{part1Name, xShift, yShift};
            assetsToFetch.push_back(part1Name);
        }

        if (fileManager->inDB(part2Name + IMAGE_EXT))
        {
            cg.part2 = Image{part2Name, xShift, yShift};
            assetsToFetch.push_back(part2Name);
        }
    }
    break;

    case IMAGE_TYPE::FW:
    {
        if (zIndex >= currFws.size())
            return;

        // Attempt to match CS2 offsets for FW images
        xShift += 90;
        yShift += 160;

        // Assume same behaviour as CG

        auto args = getAssetArgs(asset);
        if (args.size() < 5)
        {
            LOG << "Invalid FW args for '" << asset << "'";
            return;
        }

        const std::string &baseRaw = args[0];
        const std::string &baseName = baseRaw + "_" + args[1];
        const std::string &part1Name = baseRaw + "_" + Utils::zeroPad(args[3], 3);
        const std::string &part2Name = baseRaw + "_" + Utils::zeroPad(args[4], 4);

        Fw &fw = currFws[zIndex];
        fw.clear();

        if (fileManager->inDB(baseName + IMAGE_EXT))
        {
            fw.base = Image{baseName, xShift, yShift};
            assetsToFetch.push_back(baseName);
        }

        if (fileManager->inDB(part1Name + IMAGE_EXT))
        {
            fw.part1 = Image{part1Name, xShift, yShift};
            assetsToFetch.push_back(part1Name);
        }

        if (fileManager->inDB(part2Name + IMAGE_EXT))
        {
            fw.part2 = Image{part2Name, xShift, yShift};
            assetsToFetch.push_back(part2Name);
        }
    }

    break;
    }

    // Fetch and cache required assets
    for (auto &name : assetsToFetch)
    {
        // Prevent fetching already cached images
        auto pos = textureCache.find(name);
        if (pos != textureCache.end())
            continue;

        // Initialize cache entry with NULL (more efficient but prevents failed fetches from retrying)
        // textureCache[name];

        // Assume frame 0 for all images
        fileManager->fetchAssetAndProcess(name + IMAGE_EXT, this, &ImageManager::processImage, std::make_pair(name, 0));
    }
}

// Decode a raw HG buffer and cache the texture
void ImageManager::processImage(byte *buf, size_t sz, std::pair<std::string, int> nameIdx)
{
    auto &name = nameIdx.first;
    auto frameIdx = nameIdx.second;

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
