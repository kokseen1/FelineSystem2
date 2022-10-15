#include <stdio.h>
#include <stdlib.h>
#include <utils.hpp>

#include <zlib.h>

namespace Utils
{
    std::string zeroPad(std::string str, size_t len)
    {
        return std::string(len - std::min(len, str.length()), '0') + str;
    }

    // Uncompress a buffer and return it as a vector
    std::vector<byte> zlibUncompress(uint32 destLen, byte *source, uint32 &sourceLen)
    {
        std::vector<byte> dest(destLen);
        auto res = uncompress(dest.data(), &destLen, source, sourceLen);
        if (res != Z_OK)
        {
            return {};
        }

        return dest;
    }

    // Read a local file and return its contents as a vector
    std::vector<byte> readFile(const std::string fpath, long offset, long sz)
    {
        // Use C-style for compatibility
        FILE *fp = fopen(fpath.c_str(), "rb");
        if (fp == NULL)
        {
            return {};
        }

        if (offset == -1 || sz == -1)
        {
            // Read whole file
            fseek(fp, 0, SEEK_END);
            sz = ftell(fp);
            rewind(fp);
        }
        else
        {
            fseek(fp, offset, SEEK_SET);
        }

        std::vector<byte> buf(sz);
        fread(&buf[0], sizeof(byte), sz, fp);
        fclose(fp);

        return buf;
    }
}