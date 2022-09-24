#include <stdio.h>

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
    printf("ImageManager initialized\n");
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

        // Attempt to retrieve texture from cache
        auto got = textureDataCache.find(name);
        if (got != textureDataCache.end())
        {
            auto textureData = got->second;
            auto &stdinfo = textureData.second;
            renderTexture(textureData.first, stdinfo.OffsetX, stdinfo.OffsetY);

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

    if (imageData.type == IMAGE_TYPE::IMAGE_BG)
    {
        currentBg = imageData.names[0];
    }

    SDL_RenderPresent(renderer);
}

// Returns a pointer to a texture from a given frame
// Caller is responsible for freeing the texture
SDL_Texture *ImageManager::getTextureFromFrame(HGDecoder::Frame frame)
{
    auto rgbaVec = HGDecoder::getPixelsFromFrame(frame);

    // Pixel buffer must remain alive when using surface
    SDL_Surface *surface = SDL_CreateRGBSurfaceFrom(rgbaVec.data(), frame.Stdinfo->Width, frame.Stdinfo->Height, frame.Stdinfo->BitDepth, PITCH(frame.Stdinfo->Width, frame.Stdinfo->BitDepth), RMASK, GMASK, BMASK, AMASK);

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);

    SDL_FreeSurface(surface);

    return texture;
}

// Parses image arguments into ImageData to be displayed
// Names of assets must be inserted in ascending z-index
void ImageManager::setImage(std::vector<std::string> args, IMAGE_TYPE type)
{
    ImageData imageData{type};
    std::vector<std::string> &names = imageData.names;

    switch (type)
    {
    case IMAGE_TYPE::IMAGE_BG:
        names.push_back(args[ARG_BG_NAME]);
        break;
    case IMAGE_TYPE::IMAGE_CG:
        names.push_back(currentBg);
        names.push_back(args[ARG_CG_NAME] + "_" + args[ARG_CG_BODY]);
        names.push_back(args[ARG_CG_NAME] + "_" + Utils::zeroPad(args[ARG_CG_EYES], 3));
        names.push_back(args[ARG_CG_NAME] + "_" + Utils::zeroPad(args[ARG_CG_MOUTH], 4));

        // TODO: Add additional arguments like xpos, ypos, effect
        break;
    }

    displayImage(imageData);

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

    displayImage(imageData);
}