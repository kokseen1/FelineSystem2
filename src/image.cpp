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

void ImageManager::renderTexture(SDL_Texture *texture, int xpos, int ypos)
{
    SDL_Rect DestR = {xpos, ypos};
    SDL_QueryTexture(texture, NULL, NULL, &DestR.w, &DestR.h);
    SDL_RenderCopyEx(renderer, texture, NULL, &DestR, 0, 0, SDL_FLIP_VERTICAL);
}

void ImageManager::displayTexture(SDL_Texture *texture, ImageData imageData)
{
    switch (imageData.type)
    {
    case IMAGE_TYPE::IMAGE_BG:
        renderTexture(texture, 0, 0);
        currentBg = texture;
        break;
    case IMAGE_TYPE::IMAGE_CG:
        switch (imageData.subtype)
        {
        case IMAGE_SUBTYPE::IMAGE_CG_SPRITE:
            renderTexture(texture, 200, 0);
            break;
        case IMAGE_SUBTYPE::IMAGE_CG_FACE:
            renderTexture(texture, 0, 0);
            break;
        }
        break;
    }

    // Do not free as texture is stored in the cache
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

SDL_Texture *ImageManager::getCachedTexture(std::string name)
{
    auto got = textureCache.find(name);
    if (got != textureCache.end())
    {
        return got->second;
    }

    return NULL;
}

// Attempts to retrieve image from cache first, else fetch file
void ImageManager::queueImage(ImageData imageData)
{
    // Try cache first
    SDL_Texture *texture = getCachedTexture(imageData.name);
    if (texture != NULL)
    {
        displayTexture(texture, imageData);
        return;
    }

    // Fetch file if image not in cache
    auto fpath = ASSETS IMAGE_PATH + imageData.name + IMAGE_EXT;
    Utils::fetchFileAndProcess(fpath, this, &ImageManager::processImage, imageData);
}

void ImageManager::setImage(ImageData imageData)
{
    std::string &name = imageData.name;
    switch (imageData.type)
    {
    case IMAGE_TYPE::IMAGE_BG:
        name = imageData.args[ARG_BG_NAME];
        queueImage(imageData);
        break;
    case IMAGE_TYPE::IMAGE_CG:
        name = imageData.args[ARG_CG_NAME] + "_" + imageData.args[ARG_CG_SPRITE];
        imageData.subtype = IMAGE_SUBTYPE::IMAGE_CG_SPRITE;
        queueImage(imageData);

        name = imageData.args[ARG_CG_NAME] + "_" + Utils::zeroPad(imageData.args[ARG_CG_FACE], 3);
        imageData.subtype = IMAGE_SUBTYPE::IMAGE_CG_FACE;
        queueImage(imageData);
        break;
    }

    std::cout << SDL_GetTicks() << std::endl;
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
    textureCache.insert({imageData.name, texture});

    displayTexture(texture, imageData);
}