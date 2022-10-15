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

}