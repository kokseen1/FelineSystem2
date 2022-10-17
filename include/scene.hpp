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

#define SCRIPT_PATH "scene/"
#define SCRIPT_EXT ".cst"
#define SCRIPT_SIGNATURE "CatScene"
#define SCRIPT_START "com46"

#define DIALOGUE_ENABLE

class SceneManager
{
private:
    FileManager *fileManager = NULL;
    MusicManager *musicManager = NULL;
    ImageManager *imageManager = NULL;
    Parser parser;

    std::vector<std::pair<std::string, std::string>> currChoices;

    std::vector<byte> currScriptData;
    StringOffsetTable *stringOffsetTable;
    byte *stringTableBase;
    size_t stringEntryCount;
    size_t currStringEntry;

    void handleCommand(std::string);

    void loadFromBuf(byte *, size_t, int);

public:
    SceneManager(MusicManager *, ImageManager *, FileManager *);

    void parseNext();

    void setScript(const std::string);

    void selectChoice(int);
};