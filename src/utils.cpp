#include <stdio.h>
#include <stdlib.h>
#include <utils.hpp>

#include <zlib.h>

namespace Utils
{
    // Uncompress a buffer and return it as a vector
    std::vector<byte> zlibUncompress(uint32 destLen, byte *source, uint32 sourceLen)
    {
        std::vector<byte> dest(destLen);
        auto res = uncompress(&dest[0], &destLen, source, sourceLen);
        if (res != Z_OK)
        {
            printf("zlibUncompress error\n");
            return {};
        }

        return dest;
    }

    // Read a local file and return its contents as a vector
    std::vector<byte> readFile(const char *fpath)
    {
        // Use C-style for compatibility
        FILE *fp = fopen(fpath, "rb");
        if (fp == NULL)
        {
            return {};
        }

        fseek(fp, 0, SEEK_END);
        auto sz = ftell(fp);
        rewind(fp);

        std::vector<byte> buf(sz);
        fread(&buf[0], sizeof(byte), sz, fp);
        fclose(fp);

        return buf;
    }
}