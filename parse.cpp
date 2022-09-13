#include <iostream>
#include <utils.hpp>
#include <cstformat.h>
#include <memory>

#include "zlib.h"

class ScriptParser
{

private:
    CSTHeader *scriptHeader = NULL;

public:
    void readScript(char *fpath)
    {
        auto scriptBuf = Utils::getData(fpath);
        scriptHeader = (CSTHeader *)scriptBuf.data();

        printf("CompressedSize: %d\n", scriptHeader->CompressedSize);
        printf("DecompressedSize: %d\n", scriptHeader->DecompressedSize);

        if (scriptHeader->CompressedSize == 0)
        {
            parseScript(&scriptHeader->ScriptDataHeader);
        }
        else
        {
            // auto scriptDecompressedS = std::make_shared<byte>(scriptHeader->DecompressedSize);
            // auto scriptDecompressed = scriptDecompressedS.get();
            auto scriptDecompressed = new byte[scriptHeader->DecompressedSize];

            // Zlib decompress
            int res = uncompress(scriptDecompressed, &scriptHeader->DecompressedSize, (byte *)&scriptHeader->ScriptDataHeader, scriptHeader->CompressedSize);
            if (Z_OK != res)
            {
                printf("uncompress error\n");
            }

            printf("Last byte: %hhx\n", scriptDecompressed[scriptHeader->DecompressedSize - 1]);
            printf("Last char: %c\n", scriptDecompressed[scriptHeader->DecompressedSize - 1]);

            parseScript((ScriptDataHeader *)scriptDecompressed);
        }
    }

    void parseScript(ScriptDataHeader *scriptDataHeader)
    {
        StringOffsetTable *stringOffsetTable = (StringOffsetTable *)(&scriptDataHeader->TablesStart + scriptDataHeader->StringOffsetTableOffset);
        StringTable *stringTableBase = (StringTable *)(&scriptDataHeader->TablesStart + scriptDataHeader->StringTableOffset);

        auto StringEntryCount = (scriptDataHeader->StringTableOffset - scriptDataHeader->StringOffsetTableOffset) / sizeof(StringOffsetTable);

        for (int i = 0; i < StringEntryCount; i++)
        {
            printf("Offset: %02x\n", stringOffsetTable->Offset);
            StringTable *stringTable = (StringTable *)((byte *)stringTableBase + stringOffsetTable->Offset);
            printf("Type: %02x\n", stringTable->Type);
            printf("String: %s\n", &stringTable->StringStart);

            stringOffsetTable++;
        }
    }
};

int main()
{
    ScriptParser sp;

    sp.readScript((char *)R"(..\CatSystemParser\cs2_full_v301\system\scene\fes_start.cst)");

    return 0;
}