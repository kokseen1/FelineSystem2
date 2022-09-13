#include <stdio.h>
#include <stdlib.h>
#include <utils.hpp>

#include <zlib.h>

namespace Utils
{
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
    std::vector<byte> readFile(char *fpath)
    {
        // Use C-style for compatibility
        FILE *fp = fopen(fpath, "rb");
        if (fp == NULL)
        {
            printf("Could not read file %s\n", fpath);
            // Return empty vector
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

#ifdef __EMSCRIPTEN__
    // Asynchronously fetch a file via url and pass the buffer to a callback
    void readFile(char *fpath, void *arg, em_async_wget_onload_func onLoad, em_arg_callback_func onError)
    {
        emscripten_async_wget_data(fpath, arg, onLoad, onError);
    }
#endif
}