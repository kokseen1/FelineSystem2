#pragma once

#include <cstformat.h>
#include <utils.hpp>
#include <music.hpp>
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

#define LOG_CMD

typedef struct
{
    std::string scriptName;
    byte *offsetFromBase;
} SaveData;

class SceneManager
{
private:
    // Pointers to other manager classes
    FileManager *fileManager = NULL;
    MusicManager *musicManager = NULL;
    ImageManager *imageManager = NULL;

    // Recursive-descent parser for expressions
    Parser parser;

    bool parseScript = false;
    Uint64 targetTicks = 0;

    int autoMode = -1;

    int speakerCounter = 0;

    std::string currScriptName;

    // Vector of <script name, prompt> pairs
    std::vector<std::pair<std::string, std::string>> currChoices;

    // Vector containing uncompressed script data to be traversed
    std::vector<byte> currScriptData;

    StringOffsetTable *stringOffsetTable;
    byte *stringTableBase;

    SaveData saveData[10];

    void setDelay(Uint64 delay) { targetTicks = SDL_GetTicks64() + delay; }

    void handleCommand(const std::string &);

    void loadScript(byte *, size_t, const std::string &);

    void loadScriptStart(byte *, size_t, const std::string);

    void setScript(const std::string&);

    void loadScriptOffset(byte *, size_t, const SaveData);

    void setScriptOffset(const SaveData);

    int parseLine();

    std::string cleanText(const std::string &);

    std::string sj2utf8(const std::string &);

public:
    SceneManager(MusicManager *, ImageManager *, FileManager *);

    void parse()
    {
        targetTicks = 0;
        parseScript = true;
    }

    void tickScript();

    // void parseNext();

    void start();

    void selectChoice(int);

    void wait(const int);
    
    void saveState(const int);

    void loadState(const int);
};