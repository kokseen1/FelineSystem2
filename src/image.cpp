#include <image.hpp>
#include <hgdecoder.hpp>
#include <utils.hpp>
#include <sys_mwnd.h>
#include <sys_sel.h>

#include <iostream>
#include <sstream>
#include <vector>

static std::map<std::string, TextureData> textureCache;

void to_json(json &j, const Image &i)
{
    j = json{
        {KEY_NAME, i.textureName},
        {KEY_XSHIFT, i.xShift},
        {KEY_YSHIFT, i.yShift}};
}

void from_json(const json &j, Image &i)
{
    j.at(KEY_NAME).get_to(i.textureName);
    j.at(KEY_XSHIFT).get_to(i.xShift);
    j.at(KEY_YSHIFT).get_to(i.yShift);
}

void to_json(json &j, const Fw &i)
{
    j = json{
        {KEY_NAME, i.assetRaw},
        // Assume base is always valid in a valid CG/FW
        // Undo hardcoded offset for FW
        {KEY_XSHIFT, i.base.xShift - FW_XSHIFT},
        {KEY_YSHIFT, i.base.yShift - FW_YSHIFT}};
}

void to_json(json &j, const Cg &i)
{
    j = json{
        {KEY_NAME, i.assetRaw},
        // Assume base is always valid in a valid CG
        {KEY_XSHIFT, i.base.xShift},
        {KEY_YSHIFT, i.base.yShift}};
}

void from_json(const json &j, Cg &i)
{
    j.at(KEY_NAME).get_to(i.assetRaw);
    // Assume base is always valid in a valid CG
    j.at(KEY_XSHIFT).get_to(i.base.xShift);
    j.at(KEY_YSHIFT).get_to(i.base.yShift);
}

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

const json Image::dump()
{
    auto &j = *this;
    return j;
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

const json Fw::dump()
{
    auto &j = *this;
    return j;
}

const json Cg::dump()
{
    auto &j = *this;
    return j;
}

// Clear the entire canvas
void ImageManager::clearCanvas()
{
    clearImageType(IMAGE_TYPE::BG);
    clearImageType(IMAGE_TYPE::EG);
    clearImageType(IMAGE_TYPE::CG);
    clearImageType(IMAGE_TYPE::FW);
}

// Load a json object of dumped image data
// Throws json::out_of_range if key is missing
void ImageManager::loadDump(const json &j)
{
    clearCanvas();

    if (j.contains(KEY_BG))
    {
        for (auto &e : j[KEY_BG].items())
        {
            const auto &idx = std::stoi(e.key());
            auto bg = e.value().get<Bg>();
            setImage(IMAGE_TYPE::BG, idx, bg.textureName, bg.xShift, bg.yShift);
        }
    }

    if (j.contains(KEY_EG))
    {
        for (auto &e : j[KEY_EG].items())
        {
            const auto &idx = std::stoi(e.key());
            auto eg = e.value().get<Eg>();
            setImage(IMAGE_TYPE::EG, idx, eg.textureName, eg.xShift, eg.yShift);
        }
    }

    if (j.contains(KEY_CG))
    {
        for (auto &e : j[KEY_CG].items())
        {
            const auto &idx = std::stoi(e.key());
            auto cg = e.value().get<Cg>();
            setImage(IMAGE_TYPE::CG, idx, cg.assetRaw, cg.base.xShift, cg.base.yShift);
        }
    }

    if (j.contains(KEY_FW))
    {
        for (auto &e : j[KEY_FW].items())
        {
            const auto &idx = std::stoi(e.key());
            auto fw = e.value().get<Fw>();
            setImage(IMAGE_TYPE::FW, idx, fw.assetRaw, fw.base.xShift, fw.base.yShift);
        }
    }
}

// Dump all images currently on the canvas to a json object
const json ImageManager::dump()
{
    json j;

    for (int i = 0; i < currBgs.size(); i++)
    {
        if (!currBgs[i].isActive())
            continue;
        j[KEY_BG][std::to_string(i)] = currBgs[i].dump();
    }

    for (int i = 0; i < currEgs.size(); i++)
    {
        if (!currEgs[i].isActive())
            continue;
        j[KEY_EG][std::to_string(i)] = currEgs[i].dump();
    }

    for (int i = 0; i < currCgs.size(); i++)
    {
        if (!currCgs[i].isActive())
            continue;
        j[KEY_CG][std::to_string(i)] = currCgs[i].dump();
    }

    for (int i = 0; i < currFws.size(); i++)
    {
        if (!currFws[i].isActive())
            continue;
        j[KEY_FW][std::to_string(i)] = currFws[i].dump();
    }

    return j;
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

    // Decode and cache choice selection asset
    processImage(sys_sel, sizeof(sys_sel), std::pair<std::string, int>(SEL, 2));

    mwnd = {renderer, MWND, MWND_XSHIFT, MWND_YSHIFT};
    mwndDeco = {renderer, MWND_DECO, MWND_XSHIFT, MWND_YSHIFT};

    // Decode and cache message window assets
    processImage(sys_mwnd, sizeof(sys_mwnd), std::pair<std::string, int>(MWND, 43));
    processImage(sys_mwnd, sizeof(sys_mwnd), std::pair<std::string, int>(MWND_DECO, 42));
    if (textureCache[MWND].first != NULL)
        SDL_SetTextureAlphaMod(textureCache[MWND].first, MWND_ALPHA);

    LOG << "ImageManager initialized";
}

void ImageManager::toggle_fullscreen()
{
#ifdef __EMSCRIPTEN__
    // Much better for mobile
    EmscriptenFullscreenChangeEvent fsce;
    EMSCRIPTEN_RESULT ret = emscripten_get_fullscreen_status(&fsce);
    if (!fsce.isFullscreen)
    {
        ret = emscripten_request_fullscreen("#canvas", 1);
    }
    else
    {
        ret = emscripten_exit_fullscreen();
        ret = emscripten_get_fullscreen_status(&fsce);
    }
#else
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
#endif
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
        choice.render(yShift);
        yShift +=  SEL_HEIGHT + SEL_SPACING;
    }
}

// Render images in order of type precedence and z-index
void ImageManager::render()
{
    // Clear render canvas
    SDL_RenderClear(renderer);

    for (auto &bg : currBgs)
        bg.render();

    for (auto &cg : currCgs)
        cg.render();

    for (auto &eg : currEgs)
        eg.render();

    // Render message window
    mwnd.render();
    mwndDeco.render();

    for (auto &fw : currFws)
        fw.render();

    renderMessage(currText);
    renderSpeaker(currSpeaker);

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
void ImageManager::clearImageType(const IMAGE_TYPE type)
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

// Get reference to specified image type at z-index
// Reference to base image for CG/FW
const Image &ImageManager::getImage(const IMAGE_TYPE type, const int zIndex)
{
    switch (type)
    {
    case IMAGE_TYPE::BG:
        if (zIndex >= currBgs.size())
            return {};
        return currBgs[zIndex];

    case IMAGE_TYPE::EG:
        if (zIndex >= currEgs.size())
            return {};
        return currEgs[zIndex];

    // Assume that any valid CG/FW must always contain a valid base
    case IMAGE_TYPE::CG:
        if (zIndex >= currCgs.size())
            return {};
        return currCgs[zIndex].base;

    case IMAGE_TYPE::FW:
        if (zIndex >= currFws.size())
            return {};
        return currFws[zIndex].base;
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
void ImageManager::setImage(const IMAGE_TYPE type, const int zIndex, std::string asset, int xShift, int yShift)
{

#ifdef LOWERCASE_ASSETS
    // Convert names to lowercase
    Utils::lowercase(asset);
#endif

    switch (type)
    {

    case IMAGE_TYPE::BG:
        if (zIndex >= currBgs.size())
            return;
        if (!fileManager->inDB(asset + IMAGE_EXT))
            return;

        currBgs[zIndex] = {renderer, asset, xShift, yShift};
        fetchImage(asset);
        break;

    case IMAGE_TYPE::EG:
        if (zIndex >= currEgs.size())
            return;
        if (!fileManager->inDB(asset + IMAGE_EXT))
            return;

        currEgs[zIndex] = {renderer, asset, xShift, yShift};
        fetchImage(asset);
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

        // Store raw asset identifier
        cg.assetRaw = asset;

        if (fileManager->inDB(baseName + IMAGE_EXT))
        {
            cg.base = Image{renderer, baseName, xShift, yShift};
            fetchImage(baseName);
        }

        if (fileManager->inDB(part1Name + IMAGE_EXT))
        {
            cg.part1 = Image{renderer, part1Name, xShift, yShift};
            fetchImage(part1Name);
        }

        if (fileManager->inDB(part2Name + IMAGE_EXT))
        {
            cg.part2 = Image{renderer, part2Name, xShift, yShift};
            fetchImage(part2Name);
        }
    }
    break;

    case IMAGE_TYPE::FW:
    {
        if (zIndex >= currFws.size())
            return;

        // Attempt to match CS2 offsets for FW images
        xShift += FW_XSHIFT;
        yShift += FW_YSHIFT;

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

        fw.assetRaw = asset;

        if (fileManager->inDB(baseName + IMAGE_EXT))
        {
            fw.base = Image{renderer, baseName, xShift, yShift};
            fetchImage(baseName);
        }

        if (fileManager->inDB(part1Name + IMAGE_EXT))
        {
            fw.part1 = Image{renderer, part1Name, xShift, yShift};
            fetchImage(part1Name);
        }

        if (fileManager->inDB(part2Name + IMAGE_EXT))
        {
            fw.part2 = Image{renderer, part2Name, xShift, yShift};
            fetchImage(part2Name);
        }
    }
    break;
    }
}

// Helper function to fetch images that are not already in cache
void ImageManager::fetchImage(const std::string &name)
{
    // Prevent fetching already cached images
    auto pos = textureCache.find(name);
    if (pos != textureCache.end())
        return;

    // Initialize cache entry with NULL (more efficient but prevents failed fetches from retrying)
    // textureCache[name];

    // Assume frame 0 for all images
    fileManager->fetchAssetAndProcess(name + IMAGE_EXT, this, &ImageManager::processImage, std::make_pair(name, 0));
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
