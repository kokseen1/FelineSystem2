#pragma once

#include <cstformat.h>
#include <utils.hpp>
#include <audio.hpp>
#include <image.hpp>
#include <parser.hpp>
#include <file.hpp>

#include <vector>
#include <cstring>
#include <string>
#include <iostream>
#include <regex>

#define SCRIPT_EXT ".cst"
#define SCRIPT_SIGNATURE "CatScene"

#define SCRIPT_ENTRYPOINT "op"
#define WAIT_DEFAULT_DELAY 100

#define KEY_SCENE "scene"
#define KEY_IMAGE "image"
#define KEY_AUDIO "audio"
#define KEY_TEXT "text"
#define KEY_SPEAKER "speaker"
#define KEY_SYMBOL_TABLE "var"
#define KEY_SCRIPT_NAME "name"
#define KEY_OFFSET "offset"
#define KEY_CHOICE "choice"

#define LOG_CMD

typedef struct
{
    std::string scriptName;
    byte *offsetFromBase;
} SaveData;

class Choice;
class ImageManager;

class SceneManager
{
private:
    // Pointers to other manager classes
    FileManager &fileManager;
    AudioManager &audioManager;
    ImageManager &imageManager;

    // Recursive-descent parser for expressions
    Parser parser;

    bool parseScript = false;
    Uint64 targetTicks = 0;

    int autoMode = -1;

    int speakerCounter = 0;

    std::string currScriptName;

    std::vector<Choice> &currChoices;

    // Vector containing uncompressed script data to be traversed
    std::vector<byte> currScriptData;

    // StringOffsetTable *prevStringOffsetTable;
    StringOffsetTable *stringOffsetTable;
    byte *stringTableBase;

    void setDelay(Uint64 delay) { targetTicks = SDL_GetTicks64() + delay; }

    void handleCommand(const std::string &);

    void loadScript(byte *, size_t, const std::string &);

    void loadScriptStart(byte *, size_t, const std::string &);

    void setScript(const std::string &);

    void loadScriptOffset(byte *, size_t, const SaveData &);

    void setScriptOffset(const SaveData &);

    int parseLine();

    std::string cleanText(const std::string &);

    std::string sj2utf8(const std::string &);

public:
    SceneManager(AudioManager &, ImageManager &, FileManager &, std::vector<Choice>&);

    void parse()
    {
        targetTicks = 0;
        parseScript = true;
    }

    void tickScript();

    // void parseNext();

    void start();

    void selectChoice(int);

    void saveState(const int);

    void loadState(const int);
};
