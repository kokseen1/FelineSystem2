#include <iostream>
#include <vector>
#include <string.h>

#include <utils.hpp>
#include <parser.hpp>

// Read a script from a file
void ScriptParser::readFromFile(char *fpath)
{
    auto scriptBuf = Utils::readFile(fpath);
    if (scriptBuf.empty())
    {
        printf("readScript failed\n");
        return;
    }

    readFromBuf(&scriptBuf[0]);
}

// Read a script from a memory buffer
void ScriptParser::readFromBuf(byte *buf)
{
    CSTHeader *scriptHeader = reinterpret_cast<CSTHeader *>(buf);
    if (strncmp(scriptHeader->FileSignature, "CatScene", sizeof(scriptHeader->FileSignature)) != 0)
    {
        printf("Invalid CST file signature!\n");
        return;
    }

    printf("CompressedSize: %d\n", scriptHeader->CompressedSize);
    printf("DecompressedSize: %d\n", scriptHeader->DecompressedSize);

    auto *scriptData = reinterpret_cast<byte *>(scriptHeader + 1);

    if (scriptHeader->CompressedSize == 0)
    {
        // Uncompressed script
        parseScriptData(scriptData);
    }
    else
    {
        auto scriptDataDecompressed = Utils::zlibUncompress(scriptHeader->DecompressedSize, scriptData, scriptHeader->CompressedSize);
        if (scriptDataDecompressed.empty())
        {
            printf("Script uncompress error\n");
            return;
        }

        parseScriptData(&scriptDataDecompressed[0]);
    }
}

void ScriptParser::parseScriptData(byte *scriptData)
{
    ScriptDataHeader *scriptDataHeader = reinterpret_cast<ScriptDataHeader *>(scriptData);
    byte *tablesStart = reinterpret_cast<byte *>(scriptDataHeader + 1);

    StringOffsetTable *stringOffsetTable = reinterpret_cast<StringOffsetTable *>(tablesStart + scriptDataHeader->StringOffsetTableOffset);
    byte *stringTableBase = tablesStart + scriptDataHeader->StringTableOffset;

    auto stringEntryCount = (stringTableBase - reinterpret_cast<byte *>(stringOffsetTable)) / sizeof(StringOffsetTable);
    StringTable *stringTable = NULL;

    for (int i = 0; i < stringEntryCount; i++, stringOffsetTable++)
    {
        stringTable = reinterpret_cast<StringTable *>(stringTableBase + stringOffsetTable->Offset);
        printf("Type: %02x String: %s Strlen: %d\n", stringTable->Type, &stringTable->StringStart, strlen(&stringTable->StringStart));
    }
}