#include <stdio.h>
#include <scene.hpp>
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

void ImageManager::displayTexture(SDL_Texture *texture)
{
    int w, h;
    SDL_QueryTexture(texture, NULL, NULL, &w, &h);
    SDL_Rect DestR = {0, 0, w, h};
    SDL_RenderCopyEx(renderer, texture, NULL, &DestR, 0, 0, SDL_FLIP_VERTICAL);
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
    auto &fpath = imageData.fpath = ASSETS IMAGE_PATH + imageData.name + IMAGE_EXT;
    if (textureCache.find(fpath) != textureCache.end())
    {
        displayTexture(textureCache[fpath]);
        return;
    }

    Utils::fetchFileAndProcess(fpath, this, &ImageManager::processImage, imageData);
}
