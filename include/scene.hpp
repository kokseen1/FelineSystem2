#pragma once

#include <SDL2/SDL.h>
#include <asmodean.h>
#include <string>
#include <map>

#include <hgdecoder.hpp>

#define IMAGE_PATH "/image/"
#define IMAGE_EXT ".hg3"
#define IMAGE_SIGNATURE "HG-3"

#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 576

enum class IMAGE_TYPE
{
    IMAGE_BG,
    IMAGE_CG,
};

typedef struct
{
    const std::string name;
    const IMAGE_TYPE type;
    std::string fpath;

} ImageData;

class SceneManager
{

public:
    SceneManager();

    void setImage(ImageData);

    void displayTexture(SDL_Texture *);

    SDL_Texture *getTextureFromFrame(HGDecoder::Frame);

    SDL_Window *window = NULL;

    SDL_Renderer *renderer = NULL;

private:
    std::map<std::string, SDL_Texture *> textureCache;

    // Decode a raw HG buffer and display the first frame
    template <typename T>
    void processImage(byte *buf, size_t sz, T userdata)
    {
        HGHeader *hgHeader = reinterpret_cast<HGHeader *>(buf);

        // Verify signature
        if (strncmp(hgHeader->FileSignature, IMAGE_SIGNATURE, sizeof(hgHeader->FileSignature)) != 0)
        {
            printf("Invalid image file signature!\n");
            return;
        }

        // Retrieve frames
        FrameHeader *frameHeader = reinterpret_cast<FrameHeader *>(hgHeader + 1);
        std::vector<HGDecoder::Frame> frames = HGDecoder::getFrames(frameHeader);
        if (frames.empty())
        {
            printf("No frames found\n");
            return;
        }

        // Just handle the first frame for now
        SDL_Texture *texture = getTextureFromFrame(frames[0]);

        // Store texture in cache
        textureCache.insert({userdata.fpath, texture});

        // Do not free as it is in cache
        // SDL_DestroyTexture(texture);

        displayTexture(texture);
    }
};
