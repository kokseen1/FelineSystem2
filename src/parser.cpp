#include <regex>
#include <iostream>
#include <string>

#include <utils.hpp>
#include <parser.hpp>

ScriptParser::ScriptParser(MusicPlayer *mp) : musicPlayer(mp){};

void ScriptParser::setScript(const char *fpath)
{
    Utils::processFile(fpath, this, &ScriptParser::loadFromBuf);
}

// Read a script from a memory buffer
void ScriptParser::loadFromBuf(byte *buf, size_t sz)
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

void ScriptParser::parseNext()
{
    if (currStringEntry < stringEntryCount)
    {
        auto stringTable = reinterpret_cast<StringTable *>(stringTableBase + stringOffsetTable->Offset);

        stringOffsetTable++;
        currStringEntry++;

        switch (stringTable->Type)
        {
        case 0x02: // Wait for input after message
        case 0x03: // Novel page break and wait for input after message
            break;

        case 0x20: // Display a message
                   // setMessage();
        case 0x21: // Set speaker of the message
            // setSpeaker();
            printf("%s\n", &stringTable->StringStart);
            goto next;

        case 0x30: // Perform any other command
            handleCommand(&stringTable->StringStart);
            goto next;

        // Debug:
        case 0xF0: // Sets the name of the source script file
        case 0xF1: // Marks the current line number in the source script file (as a string)

        next:
            // Might want to avoid recursion
            parseNext();
        }
    }
    else
    {
        printf("End of script!\n");
    }
}

void ScriptParser::handleCommand(std::string cmdString)
{
    std::smatch matches;
    if (std::regex_match(cmdString, matches, std::regex("^pcm (.+)")))
    {
        if (matches.size() == 2)
        {
            musicPlayer->playPcm(matches[1].str());
        }
    }
}