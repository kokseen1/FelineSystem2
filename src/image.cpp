#include <image.hpp>
#include <hgdecoder.hpp>
#include <utils.hpp>
#include <sys_mwnd.h>

#include <iostream>
#include <sstream>
#include <vector>

ImageManager::ImageManager(FileManager *fm) : fileManager{fm}
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        throw std::runtime_error("Unable to initialize SDL");
    }

    // Create SDL window and renderer
    window = SDL_CreateWindow("FelineSystem2",
                              SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              WINDOW_WIDTH,
                              WINDOW_HEIGHT,
                              SDL_WINDOW_SHOWN);

    setWindowIcon(window);

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_RenderSetLogicalSize(renderer, WINDOW_WIDTH, WINDOW_HEIGHT);

    if (TTF_Init() == -1)
    {
        LOG << "Failed to init TTF";
    }

    font = TTF_OpenFont(FONT_PATH, FONT_SIZE);
    if (font == NULL)
    {
        throw std::runtime_error("Cannot find font!");
    }

    // Decode and cache message window assets
    processImage(sys_mwnd, sizeof(sys_mwnd), std::pair<std::string, int>("sys_mwnd_42", 42));
    processImage(sys_mwnd, sizeof(sys_mwnd), std::pair<std::string, int>("sys_mwnd_43", 43));

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
    SDL_SetWindowIcon(window, surface);
    SDL_FreeSurface(surface);
}

// Render a texture onto the renderer at given position
void ImageManager::renderTexture(SDL_Texture *texture, int xpos, int ypos)
{
    SDL_Rect DestR = {xpos, ypos};
    SDL_QueryTexture(texture, NULL, NULL, &DestR.w, &DestR.h);
    SDL_RenderCopyEx(renderer, texture, NULL, &DestR, 0, 0, RENDERER_FLIP_MODE);
}

// Render assets specified by ImageData and display the image
// Will first attempt to retrieve textures from cache
// Otherwise, asset file will be fetched
// Aysnchronously render and display every asset available even when some are not
void ImageManager::renderImage(const ImageData &imageData)
{
    for (int i = 0; i < imageData.names.size(); i++)
    {
        auto &name = imageData.names[i];
        if (name.empty())
        {
            LOG << "Skipping empty name";
            continue;
        }

        // Attempt to retrieve texture from cache
        auto got = textureDataCache.find(name);
        if (got != textureDataCache.end())
        {
            auto textureData = got->second;
            auto texture = textureData.first;
            if (texture == NULL)
            {
                // LOG << "Missing: " << name;
                continue;
            }

            // LOG << "Render: " << name;

            auto &stdinfo = textureData.second;
            auto xPos = stdinfo.OffsetX - stdinfo.BaseX + imageData.xShift;
            auto yPos = stdinfo.OffsetY - stdinfo.BaseY + imageData.yShift;

            renderTexture(texture, xPos, yPos);
        }
        else
        {
            // Fetch file if not in cache
            // Set empty entry in cache to signify that request has been made
            textureDataCache.insert({name, std::make_pair(static_cast<SDL_Texture *>(NULL), Stdinfo{})});

            // Just handle the first frame for now
            fileManager->fetchAssetAndProcess(name + IMAGE_EXT, this, &ImageManager::processImage, std::pair<std::string, int>(name, 0));
        }
    }
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

// Retrieve and render message window textures
void ImageManager::renderMessageWindow()
{
    // Message window bg
    auto got = textureDataCache.find("sys_mwnd_43");
    if (got != textureDataCache.end())
    {
        auto &textureData = got->second;
        auto &texture = textureData.first;
        if (texture != NULL)
        {
            auto &stdinfo = textureData.second;
            SDL_SetTextureAlphaMod(texture, 120);
            renderTexture(texture, (WINDOW_WIDTH - stdinfo.Width) / 2, WINDOW_HEIGHT - stdinfo.Height);
        }
    }

    // Message window deco
    got = textureDataCache.find("sys_mwnd_42");
    if (got != textureDataCache.end())
    {
        auto &textureData = got->second;
        auto &texture = textureData.first;
        if (texture != NULL)
        {
            auto &stdinfo = textureData.second;
            renderTexture(texture, (WINDOW_WIDTH - stdinfo.Width) / 2, WINDOW_HEIGHT - stdinfo.Height);
        }
    }
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

void ImageManager::displayAll()
{
    // Clear render canvas
    SDL_RenderClear(renderer);

    // Render images in order of z-index
    for (auto &imageData : currBgs)
        renderImage(imageData);

    for (auto &imageData : currCgs)
        renderImage(imageData);

    for (auto &imageData : currEgs)
        renderImage(imageData);

    // Render message window
    renderMessageWindow();

    for (auto &imageData : currFws)
        renderImage(imageData);

    renderMessage(currText);
    renderSpeaker(currSpeaker);

    // Update screen
    SDL_RenderPresent(renderer);
}

// Clear image of type at specified z index
void ImageManager::clearZIndex(IMAGE_TYPE type, int zIndex)
{
    if (zIndex >= Z_INDEX_MAX)
    {
        LOG << "Attempted to clear at out of bounds z-index!";
        return;
    }

    switch (type)
    {
    case IMAGE_TYPE::CG:
        // LOG << "CLEARING CG" << zIndex;
        currCgs[zIndex].names.clear();
        break;
    case IMAGE_TYPE::EG:
        // LOG << "CLEARING EG" << zIndex;
        currEgs[zIndex].names.clear();
        break;
    case IMAGE_TYPE::BG:
        // LOG << "CLEARING BG" << zIndex;
        currBgs[zIndex].names.clear();
        break;
    case IMAGE_TYPE::FW:
        currFws[zIndex].names.clear();
        break;
    }
}

// Clear all layers of specified image type
void ImageManager::clearImageType(IMAGE_TYPE type)
{
    for (int i = 0; i < Z_INDEX_MAX; i++)
    {
        clearZIndex(type, i);
    }
}

// Get ImageData of specified image type at z-index
ImageData ImageManager::getImageData(IMAGE_TYPE type, int zIndex)
{
    if (zIndex >= Z_INDEX_MAX)
    {
        LOG << "Attempted get image at out of bounds z-index!";
        return {};
    }

    switch (type)
    {
    case IMAGE_TYPE::BG:
        return currBgs[zIndex];
    case IMAGE_TYPE::CG:
        return currCgs[zIndex];
    case IMAGE_TYPE::EG:
        return currEgs[zIndex];
    case IMAGE_TYPE::FW:
        return currFws[zIndex];
    }
}

// Parses image arguments into ImageData to be displayed
// Names of assets must be inserted in ascending z-index
void ImageManager::setImage(IMAGE_TYPE type, int zIndex, std::string asset, int xShift, int yShift)
{
    if (zIndex >= Z_INDEX_MAX)
    {
        LOG << "Attempted set image at out of bounds z-index!";
        return;
    }

#ifdef LOWERCASE_ASSETS
    Utils::lowercase(asset);
#endif
    ImageData id{xShift, yShift};
    switch (type)
    {

    case IMAGE_TYPE::BG:
        if (fileManager->inDb(asset + IMAGE_EXT))
        {
            id.names.push_back(asset);
            currBgs[zIndex] = id;
        }
        break;

    case IMAGE_TYPE::EG:
        if (fileManager->inDb(asset + IMAGE_EXT))
        {
            id.names.push_back(asset);
            currEgs[zIndex] = id;
        }
        break;

    case IMAGE_TYPE::FW:
        // Attempt to match CS2 offsets for FW images
        id.xShift += 90;
        id.yShift += 160;
    case IMAGE_TYPE::CG:
        std::stringstream ss(asset);
        std::vector<std::string> args;

        while (ss.good())
        {
            std::string arg;
            getline(ss, arg, ',');
            args.push_back(arg);
        }

        if (args.size() < 5)
        {
            LOG << "Invalid CG args for '" << asset << "'";
            return;
        }

        std::string &cgBase = args[0];
        std::string cg1 = cgBase + "_" + args[1];
        std::string cg2 = cgBase + "_" + Utils::zeroPad(args[3], 3);
        std::string cg3 = cgBase + "_" + Utils::zeroPad(args[4], 4);

        if (fileManager->inDb(cg1 + IMAGE_EXT))
            id.names.push_back(cg1);
        if (fileManager->inDb(cg2 + IMAGE_EXT))
            id.names.push_back(cg2);
        if (fileManager->inDb(cg3 + IMAGE_EXT))
            id.names.push_back(cg3);

        if (!id.names.empty())
        {
            switch (type)
            {
            case IMAGE_TYPE::CG:
                currCgs[zIndex] = id;
                break;
            case IMAGE_TYPE::FW:
                currFws[zIndex] = id;
                break;
            }
        }

        break;
    }

    // if (!id.names.empty())
    // displayAll();
}

// Decode a raw HG buffer and display the specified frame
void ImageManager::processImage(byte *buf, size_t sz, std::pair<std::string, int> nameIdx)
{
    auto &name = nameIdx.first;
    auto frameIdx = nameIdx.second;

    HGHeader *hgHeader = reinterpret_cast<HGHeader *>(buf);

    // Verify signature
    if (strncmp(hgHeader->FileSignature, IMAGE_SIGNATURE, sizeof(hgHeader->FileSignature)) != 0)
    {
        LOG << "Invalid image file signature!";
        return;
    }

    // Retrieve frames
    FrameHeader *frameHeader = reinterpret_cast<FrameHeader *>(hgHeader + 1);
    std::vector<HGDecoder::Frame> frames = HGDecoder::getFrames(frameHeader);
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
    textureDataCache[name] = std::make_pair(texture, *frame.Stdinfo);

    LOG << "Cached: " << name << "[" << frameIdx << "]";

    // displayAll();
}
