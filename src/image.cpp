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

void ImageManager::displayTexture(SDL_Texture *texture, ImageData userdata)
{
    SDL_Rect DestR = {0, 0};

    if (userdata.type == IMAGE_TYPE::IMAGE_BG)
    {
        currentBg = texture;
    }
    else
    {
        SDL_QueryTexture(currentBg, NULL, NULL, &DestR.w, &DestR.h);
        SDL_RenderCopyEx(renderer, currentBg, NULL, &DestR, 0, 0, SDL_FLIP_VERTICAL);
    }

    SDL_QueryTexture(texture, NULL, NULL, &DestR.w, &DestR.h);
    SDL_RenderCopyEx(renderer, texture, NULL, &DestR, 0, 0, SDL_FLIP_VERTICAL);

    // Do not free as it is in cache
    // SDL_DestroyTexture(texture);

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

void ImageManager::setImage(ImageData imageData)
{
    auto &fpath = imageData.fpath;
    switch (imageData.type)
    {
    case IMAGE_TYPE::IMAGE_BG:
        fpath = imageData.args[ARG_BG_NAME];
        break;
    case IMAGE_TYPE::IMAGE_CG:
        fpath = imageData.args[ARG_CG_NAME] + "_" + imageData.args[ARG_CG_ID];
        break;
    }
    fpath = ASSETS IMAGE_PATH + fpath + IMAGE_EXT;
    if (textureCache.find(fpath) != textureCache.end())
    {
        displayTexture(textureCache[fpath], imageData);
        return;
    }

    Utils::fetchFileAndProcess(fpath, this, &ImageManager::processImage, imageData);
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
    SDL_Texture *texture = getTextureFromFrame(frames[0]);

    // Store texture in cache
    textureCache.insert({imageData.fpath, texture});

    displayTexture(texture, imageData);
}