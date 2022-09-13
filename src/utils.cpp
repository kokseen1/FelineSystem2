#include <stdio.h>
#include <stdlib.h>
#include <utils.hpp>

namespace Utils
{
    std::vector<char> getData(char *fpath)
    {
        FILE *fp = fopen(fpath, "rb");
        if (fp == NULL)
        {
            printf("Could not read file %s\n", fpath);
            return {};
        }
        fseek(fp, 0, SEEK_END);
        auto sz = ftell(fp);
        rewind(fp);
        // char *buf = (char *)malloc(sz * sizeof(char));
        std::vector<char> buf(sz);
        fread(&buf[0], sizeof(char), sz, fp);
        fclose(fp);

        return buf;
    }

#ifdef __EMSCRIPTEN__
    void getData(char *fpath, void *arg, em_async_wget_onload_func onLoad, em_arg_callback_func onError)
    {
        emscripten_async_wget_data(fpath, arg, onLoad, onError);
    }
#endif
}