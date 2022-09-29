#include <iostream>
#include <sstream>

#include <image.hpp>
#include <hgdecoder.hpp>
#include <utils.hpp>

// Create SDL window and renderer
ImageManager::ImageManager()
{
    window = SDL_CreateWindow("FelineSystem2",
                              SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              WINDOW_WIDTH,
                              WINDOW_HEIGHT,
                              SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 100);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    if (TTF_Init() == -1)
    {
        LOG << "Failed to init TTF";
    }

    font = TTF_OpenFont(FONT_PATH, 24);
    if (font == NULL)
    {
        LOG << "Cannot find font!";
    }

    std::cout << "ImageManager initialized" << std::endl;
}

// Render a texture onto the renderer at given position
void ImageManager::renderTexture(SDL_Texture *texture, int xpos, int ypos)
{
    SDL_Rect DestR = {xpos, ypos};
    SDL_QueryTexture(texture, NULL, NULL, &DestR.w, &DestR.h);
    SDL_RenderCopyEx(renderer, texture, NULL, &DestR, 0, 0, SDL_FLIP_VERTICAL);
}

// Render assets specified by ImageData and display the image
// Will first attempt to retrieve textures from cache
// Otherwise, asset file will be fetched
// Aysnchronously render and display every asset available even when some are not
void ImageManager::displayImage(ImageData imageData)
{
    for (int i = 0; i < imageData.names.size(); i++)
    {
        auto &name = imageData.names[i];
        if (name.empty())
        {
            std::cout << "Skipping empty name" << std::endl;
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
                LOG << "Missing: " << name;
                continue;
            }

            LOG << "Render: " << name;

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

            const std::string &fpath = ASSETS IMAGE_PATH + name + IMAGE_EXT;
            Utils::fetchFileAndProcess(fpath, this, &ImageManager::processImage, name);
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

void ImageManager::setText(std::string text)
{
    SDL_Color color = {255, 255, 255, 0};
    SDL_Surface *surface = TTF_RenderText_Solid_Wrapped(font, text.c_str(), color, WINDOW_WIDTH - TEXT_XPOS);
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

    SDL_Rect rect = {TEXT_XPOS, WINDOW_HEIGHT - TEXT_HEIGHT, surface->w, surface->h};
    SDL_RenderDrawRect(renderer, &rect);
    SDL_RenderFillRect(renderer, &rect);
    SDL_RenderCopy(renderer, texture, NULL, &rect);
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

void ImageManager::displayAll()
{
    SDL_RenderClear(renderer);

    for (auto &imageData : currBgs)
        displayImage(imageData);

    for (auto &imageData : currSprites)
        displayImage(imageData);

    for (auto &imageData : currEgs)
        displayImage(imageData);

    setText(currText);

    SDL_RenderPresent(renderer);
}

void ImageManager::clearZIndex(IMAGE_TYPE type, int zIndex)
{
    if (zIndex >= Z_INDEX_MAX)
    {
        std::cout << "Attempted to clear at out of bounds z-index!" << std::endl;
        return;
    }

    switch (type)
    {
    case IMAGE_TYPE::IMAGE_CG:
        currSprites[zIndex].names.clear();
        break;
    case IMAGE_TYPE::IMAGE_EG:
        currEgs[zIndex].names.clear();
        break;
    case IMAGE_TYPE::IMAGE_BG:
        currBgs[zIndex].names.clear();
        break;
    }
}

void ImageManager::clearAllImage(IMAGE_TYPE type)
{
    for (int i = 0; i < Z_INDEX_MAX; i++)
    {
        clearZIndex(type, i);
    }
}

// Parses image arguments into ImageData to be displayed
// Names of assets must be inserted in ascending z-index
void ImageManager::setImage(IMAGE_TYPE type, int zIndex, std::string asset, int xShift, int yShift)
{
    if (zIndex >= Z_INDEX_MAX)
    {
        std::cout << "Attempted set image at out of bounds z-index!" << std::endl;
        return;
    }

    ImageData id{xShift, yShift};
    switch (type)
    {

    case IMAGE_TYPE::IMAGE_BG:
        id.names.push_back(asset);
        currBgs[zIndex] = id;
        break;

    case IMAGE_TYPE::IMAGE_EG:
        id.names.push_back(asset);
        currEgs[zIndex] = id;
        break;

    case IMAGE_TYPE::IMAGE_CG:
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
            std::cout << "Invalid CG args for '" << asset << "'" << std::endl;
            return;
        }

        std::string &cgBase = args[0];
        id.names.push_back(cgBase + "_" + args[1]);
        id.names.push_back(cgBase + "_" + Utils::zeroPad(args[3], 3));
        id.names.push_back(cgBase + "_" + Utils::zeroPad(args[4], 4));

        currSprites[zIndex] = id;
        break;
    }

    // Used for synchronization
    // std::cout << SDL_GetTicks() << std::endl;

    LOG << "Queued: " << asset << " @ " << zIndex;

    displayAll();
}

// Decode a raw HG buffer and display the first frame
void ImageManager::processImage(byte *buf, size_t sz, std::string name)
{
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

    // Just handle the first frame for now
    auto &frame = frames[0];

    SDL_Texture *texture = getTextureFromFrame(frame);

    // Store texture in cache

    textureDataCache[name] = std::make_pair(texture, *frame.Stdinfo);

    LOG << "Cached: " << name;

    displayAll();
}