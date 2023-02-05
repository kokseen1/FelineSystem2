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

// Should always be start.cst
#define SCRIPT_ENTRYPOINT "start"

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
    std::deque<json> stateHistory;

    unsigned int sectionRdraw = 0;
    Uint64 maxWaitFramestamp = 0;

    // Takes a callback and calls it on every cmdString in the current script
    // Callback should return false to break
    template <typename Functor>
    void iterateScript(Functor functor)
    {
        ScriptDataHeader *scriptDataHeader = reinterpret_cast<ScriptDataHeader *>(currScriptData.data());
        byte *tablesStart = reinterpret_cast<byte *>(scriptDataHeader + 1);

        // Locate offset table and string table
        StringOffsetTable *stringOffsetTable = reinterpret_cast<StringOffsetTable *>(tablesStart + scriptDataHeader->StringOffsetTableOffset);
        byte *stringTableBase = tablesStart + scriptDataHeader->StringTableOffset;

        // Check if pointer exceeded end of script
        while (reinterpret_cast<byte *>(stringOffsetTable) < stringTableBase)
        {
            auto stringTable = reinterpret_cast<StringTable *>(stringTableBase + stringOffsetTable->Offset);

            stringOffsetTable++;

            if (stringTable->Type != 0x30)
                continue;

            const auto &cmdString = std::string(&stringTable->StringStart);

            if (functor(cmdString) == false)
                return;
        }
    }

    void wait();
    void addSectionFrames(unsigned int);

    bool canProceed();
    bool rdrawWaited();

    void prefetch(std::vector<byte>);

    // Pointers to other manager classes
    FileManager &fileManager;
    AudioManager &audioManager;
    ImageManager &imageManager;

    // Recursive-descent parser for expressions
    Parser parser;

    bool parseScript = false;
    Uint64 waitTargetFrames = 0;

    int autoMode = -1;

    int speakerCounter = 0;

    std::string currScriptName;

    std::vector<Choice> &currChoices;

    // Vector containing uncompressed script data to be traversed
    std::vector<byte> currScriptData;

    StringOffsetTable *stringOffsetTable;
    byte *stringTableBase;

    json getCurrentState();
    void loadStateJson(const json &);

    void wait(unsigned int);

    void handleCommand(const std::string &);

    void loadScript(byte *, size_t, const std::string &);

    void loadScriptStart(byte *, size_t, const std::string &);

    void setScript(const std::string &);

    void loadScriptOffset(byte *, size_t, const SaveData &);

    void setScriptOffset(const SaveData &);

    void parseLine();

    std::string cleanText(const std::string &);

    std::string sj2utf8(const std::string &);

public:
    std::deque<std::string> toPrefetch;

    SceneManager(AudioManager &, ImageManager &, FileManager &, std::vector<Choice> &);

    void prevScene();
    void nextScene();

    void parse();
    void back();

    void tickScript();

    void start();

    void selectChoice(int);

    void saveState(const int);

    void loadState(const int);
};
