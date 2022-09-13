#include <iostream>
#include <utils.hpp>
#include <cstformat.h>
#include <vector>

#include "zlib.h"

class ScriptParser
{

private:
    CSTHeader *scriptHeader = NULL;

public:
    void readScript(char *fpath)
    {
        auto scriptBuf = Utils::getData(fpath);
        if (scriptBuf.empty())
        {
            printf("readScript failed\n");
            return;
        }
        scriptHeader = (CSTHeader *)scriptBuf.data();

        printf("CompressedSize: %d\n", scriptHeader->CompressedSize);
        printf("DecompressedSize: %d\n", scriptHeader->DecompressedSize);

        if (scriptHeader->CompressedSize == 0)
        {
            parseScript(&scriptHeader->ScriptDataHeader);
        }
        else
        {
            // auto scriptDecompressed = std::make_unique<byte[]>(scriptHeader->DecompressedSize);
            std::vector<byte> scriptDecompressed(scriptHeader->DecompressedSize);

            int res = uncompress(&scriptDecompressed[0], &scriptHeader->DecompressedSize, (byte *)&scriptHeader->ScriptDataHeader, scriptHeader->CompressedSize);
            if (Z_OK != res)
            {
                printf("uncompress error\n");
            }

            // printf("Last byte: %hhx\n", scriptDecompressed[scriptHeader->DecompressedSize - 1]);
            // printf("Last char: %c\n", scriptDecompressed[scriptHeader->DecompressedSize - 1]);

            parseScript((ScriptDataHeader *)&scriptDecompressed[0]);
        }
    }

    void parseScript(ScriptDataHeader *scriptDataHeader)
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
};

int main(int argc, char **argv)
{
    ScriptParser sp;

    sp.readScript(argv[1]);

    return 0;
}