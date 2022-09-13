#include <iostream>
#include <vector>

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
void ScriptParser::readFromBuf(void *buf)
{
    CSTHeader *scriptHeader = (CSTHeader *)buf;
    if (strcmp(scriptHeader->FileSignature, "CatScene") != 0)
    {
        printf("Invalid CST file signature!\n");
        return;
    }

    printf("CompressedSize: %d\n", scriptHeader->CompressedSize);
    printf("DecompressedSize: %d\n", scriptHeader->DecompressedSize);

    if (scriptHeader->CompressedSize == 0)
    {
        // Uncompressed script
        parseScript(&scriptHeader->ScriptDataHeader);
    }
    else
    {
        auto scriptDataDecompressed = Utils::zlibUncompress(scriptHeader->DecompressedSize, (byte *)&scriptHeader->ScriptDataHeader, scriptHeader->CompressedSize);
        if (scriptDataDecompressed.empty())
        {
            printf("Script uncompress error\n");
            return;
        }

        // printf("Last byte: %hhx\n", scriptDataDecompressed[scriptHeader->DecompressedSize - 1]);
        // printf("Last char: %c\n", scriptDataDecompressed[scriptHeader->DecompressedSize - 1]);

        parseScript((ScriptDataHeader *)&scriptDataDecompressed[0]);
    }
}

void ScriptParser::parseScript(ScriptDataHeader *scriptDataHeader)
{
    StringOffsetTable *stringOffsetTable = (StringOffsetTable *)(&scriptDataHeader->TablesStart + scriptDataHeader->StringOffsetTableOffset);
    StringTable *stringTableBase = (StringTable *)(&scriptDataHeader->TablesStart + scriptDataHeader->StringTableOffset);

    auto StringEntryCount = (scriptDataHeader->StringTableOffset - scriptDataHeader->StringOffsetTableOffset) / sizeof(StringOffsetTable);

    for (int i = 0; i < StringEntryCount; i++)
    {
        // printf("Offset: %02x\n", stringOffsetTable->Offset);
        StringTable *stringTable = (StringTable *)((byte *)stringTableBase + stringOffsetTable->Offset);
        // printf("Type: %02x\n", stringTable->Type);

        // printf("Type: %02x String: %s Strlen: %d\n", stringTable->Type, &stringTable->StringStart, strlen(&stringTable->StringStart));

        stringOffsetTable++;
    }
}