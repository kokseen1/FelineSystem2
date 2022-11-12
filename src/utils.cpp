#include <utils.hpp>

#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>

namespace Utils
{
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
