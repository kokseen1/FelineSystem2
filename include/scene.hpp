#pragma once

#include <SDL2/SDL.h>
#include <asmodean.h>
#include <string>
#include <map>

#include <hgdecoder.hpp>

#define FMT_SCENE ASSETS "/image/bg%02d.hg3"
#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 576

class SceneManager
{

public:
    SceneManager();

    void setScene(const std::string);

    void displayTexture(SDL_Texture *);

    SDL_Texture *getTextureFromFrame(HGDecoder::Frame);

    SDL_Window *window = NULL;

    SDL_Renderer *renderer = NULL;

private:
    typedef struct
    {
        const std::string &fpath;
    } ImageData;

    std::map<std::string, SDL_Texture *> textureCache;

    // Decode a raw HG buffer and display the first frame
    template <typename T>
    void displayImage(byte *buf, size_t sz, T userdata)
    {
        auto fpath = userdata.fpath;
        HGHeader *hgHeader = reinterpret_cast<HGHeader *>(buf);
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
        textureCache.insert({fpath, texture});

        // Do not free as it is in cache
        // SDL_DestroyTexture(texture);

        displayTexture(texture);
    }
};
