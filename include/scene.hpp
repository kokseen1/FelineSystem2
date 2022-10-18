#pragma once

#include <cstformat.h>

#include <vector>
#include <cstring>
#include <string>
#include <iostream>
#include <regex>

#include <utils.hpp>
#include <music.hpp>
#include <image.hpp>
#include <parser.hpp>
#include <file.hpp>

#define SCRIPT_EXT ".cst"
#define SCRIPT_SIGNATURE "CatScene"
#define SCRIPT_START "op"

// #define DIALOGUE_ENABLE
// #define LOG_CMD

class SceneManager
{
private:
    Parser parser;
    FileManager *fileManager = NULL;
    MusicManager *musicManager = NULL;
    ImageManager *imageManager = NULL;

    std::vector<std::pair<std::string, std::string>> currChoices;
    std::vector<byte> currScriptData;
    StringOffsetTable *stringOffsetTable;
    byte *stringTableBase;
    size_t stringEntryCount;
    size_t currStringEntry;

    void handleCommand(std::string);

    void loadScript(byte *, size_t, int);

    void setScript(const std::string);

public:
    SceneManager(MusicManager *, ImageManager *, FileManager *);

    void parseNext();

    void start();

    void selectChoice(int);
};