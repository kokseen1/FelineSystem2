#include <cstformat.h>

#include <vector>
#include <cstring>
#include <string>
#include <iostream>

#include <utils.hpp>
#include <music.hpp>
#include <scene.hpp>

class ScriptParser
{
private:
    MusicPlayer *musicPlayer;
    SceneManager *sceneManager;

    std::vector<byte> currScriptData;
    StringOffsetTable *stringOffsetTable;
    byte *stringTableBase;
    size_t stringEntryCount;
    size_t currStringEntry;

    void handleCommand(std::string);

    // Read a script from a memory buffer
    template <typename T>
    void loadFromBuf(byte *buf, size_t sz, T userdata)
    {
        CSTHeader *scriptHeader = reinterpret_cast<CSTHeader *>(buf);

        // Verify signature
        if (strncmp(scriptHeader->FileSignature, "CatScene", sizeof(scriptHeader->FileSignature)) != 0)
        {
            printf("Invalid CST file signature!\n");
            return;
        }

        // Get pointer to start of raw data
        auto *scriptDataRaw = reinterpret_cast<byte *>(scriptHeader + 1);

        if (scriptHeader->CompressedSize == 0)
        {
            // Uncompressed script
            currScriptData = std::vector<byte>(scriptDataRaw, scriptDataRaw + scriptHeader->DecompressedSize);
        }
        else
        {
            auto scriptDataDecompressed = Utils::zlibUncompress(scriptHeader->DecompressedSize, scriptDataRaw, scriptHeader->CompressedSize);
            if (scriptDataDecompressed.empty())
            {
                printf("Script uncompress error\n");
                return;
            }
            currScriptData = scriptDataDecompressed;
        }

        // Init script data

        ScriptDataHeader *scriptDataHeader = reinterpret_cast<ScriptDataHeader *>(currScriptData.data());
        byte *tablesStart = reinterpret_cast<byte *>(scriptDataHeader + 1);

        // Locate offset table and string table
        stringOffsetTable = reinterpret_cast<StringOffsetTable *>(tablesStart + scriptDataHeader->StringOffsetTableOffset);
        stringTableBase = tablesStart + scriptDataHeader->StringTableOffset;

        // Calculate entry count
        stringEntryCount = (stringTableBase - reinterpret_cast<byte *>(stringOffsetTable)) / sizeof(StringOffsetTable);
        currStringEntry = 0;
    }

public:
    ScriptParser(MusicPlayer *, SceneManager *);

    void parseNext();

    void setScript(const std::string);
};