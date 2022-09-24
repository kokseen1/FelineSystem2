#include <iostream>

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
            continue;
        }

        // Attempt to retrieve texture from cache
        auto got = textureDataCache.find(name);
        if (got != textureDataCache.end())
        {
            auto textureData = got->second;
            auto &texture = textureData.first;
            auto &stdinfo = textureData.second;
            if (texture == NULL)
            {
                continue;
            }

            int xShift = imageData.xShift;
            int yShift = imageData.yShift;

            auto xPos = stdinfo.OffsetX - stdinfo.BaseX + xShift;
            auto yPos = stdinfo.OffsetY - stdinfo.BaseY + yShift;

            renderTexture(texture, xPos, yPos);

            // std::cout << "totalWidth: " << stdinfo.TotalWidth << std::endl;
            // std::cout << "totalHeight: " << stdinfo.TotalHeight << std::endl;

            // std::cout << "OffsetX: " << stdinfo.OffsetX << std::endl;
            // std::cout << "OffsetY: " << stdinfo.OffsetY << std::endl;

            // std::cout << "BaseX: " << stdinfo.BaseX << std::endl;
            // std::cout << "BaseY: " << stdinfo.BaseY << std::endl;
        }
        else
        {
            // Fetch file if not in cache
            imageData.nameIdx = i;
            auto fpath = ASSETS IMAGE_PATH + name + IMAGE_EXT;
            Utils::fetchFileAndProcess(fpath, this, &ImageManager::processImage, imageData);
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
        return NULL;
    }

    // Pixel buffer must remain alive when using surface
    SDL_Surface *surface = SDL_CreateRGBSurfaceFrom(rgbaVec.data(), frame.Stdinfo->Width, frame.Stdinfo->Height, frame.Stdinfo->BitDepth, PITCH(frame.Stdinfo->Width, frame.Stdinfo->BitDepth), RMASK, GMASK, BMASK, AMASK);

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);

    SDL_FreeSurface(surface);

    return texture;
}

void ImageManager::displayAll()
{
    SDL_RenderClear(renderer);

    for (auto &imageData : currBgs)
        if (!imageData.names.empty())
            displayImage(imageData);

    for (auto &imageData : currSprites)
        if (!imageData.names.empty())
            displayImage(imageData);

    for (auto &imageData : currEgs)
        if (!imageData.names.empty())
            displayImage(imageData);

    SDL_RenderPresent(renderer);
}

// Parses image arguments into ImageData to be displayed
// Names of assets must be inserted in ascending z-index
void ImageManager::setImage(std::vector<std::string> args, IMAGE_TYPE type)
{
    ImageData imageData{type};
    std::vector<std::string> &names = imageData.names;

    switch (type)
    {
    case IMAGE_TYPE::IMAGE_EG:
    {
        names.push_back(args[ARG_EG_NAME]);
        imageData.xShift = 0;
        imageData.yShift = 0;
        currEgs[std::stoi(args[ARG_EG_Z_INDEX])] = imageData;
        break;
    }
    case IMAGE_TYPE::IMAGE_BG:
        if (args[ARG_BG_NAME] != "0" and args[ARG_BG_NAME] != "move")
        {
            names.push_back(args[ARG_BG_NAME]);

            imageData.xShift = 0;
            imageData.yShift = 0;
        }

        currBgs[std::stoi(args[ARG_BG_Z_INDEX])] = imageData;
        break;
    case IMAGE_TYPE::IMAGE_CG:
        if (!args[ARG_CG_NAME].empty())
        {
            names.push_back(args[ARG_CG_NAME] + "_" + args[ARG_CG_BODY]);
            names.push_back(args[ARG_CG_NAME] + "_" + Utils::zeroPad(args[ARG_CG_EYES], 3));
            names.push_back(args[ARG_CG_NAME] + "_" + Utils::zeroPad(args[ARG_CG_MOUTH], 4));

            imageData.xShift = 512;
            imageData.yShift = 576;
        }
        else if (!args[ARG_CG_SPEC].empty())
        {
            // Handle effect here
            // TODO: Add additional arguments like xpos, ypos, effect
            return;
        }

        currSprites[std::stoi(args[ARG_CG_Z_INDEX])] = imageData;

        break;
    }

    displayAll();

    // Used for synchronization
    // std::cout << SDL_GetTicks() << std::endl;
}

// Decode a raw HG buffer and display the first frame
void ImageManager::processImage(byte *buf, size_t sz, ImageData imageData)
{
    HGHeader *hgHeader = reinterpret_cast<HGHeader *>(buf);

    // Verify signature
    if (strncmp(hgHeader->FileSignature, IMAGE_SIGNATURE, sizeof(hgHeader->FileSignature)) != 0)
    {
        std::cout << "Invalid image file signature!" << std::endl;
        return;
    }

    // Retrieve frames
    FrameHeader *frameHeader = reinterpret_cast<FrameHeader *>(hgHeader + 1);
    std::vector<HGDecoder::Frame> frames = HGDecoder::getFrames(frameHeader);
    if (frames.empty())
    {
        std::cout << "No frames found" << std::endl;
        return;
    }

    // Just handle the first frame for now
    auto &frame = frames[0];

    SDL_Texture *texture = getTextureFromFrame(frame);

    // Store texture in cache
    textureDataCache.insert({imageData.names[imageData.nameIdx], std::make_pair(texture, *frame.Stdinfo)});

    displayAll();
}