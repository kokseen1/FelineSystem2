#include <stdio.h>
#include <stdlib.h>
#include <utils.hpp>

namespace Utils
{
    std::vector<char> getData(char *fpath)
    {
        // Does not work well with emscripten
        // std::ifstream hgfile(fpath, std::ios_base::binary);
        // std::vector<char> buffer((std::istreambuf_iterator<char>(hgfile)),
        //                          (std::istreambuf_iterator<char>()));
        // displayFrame(buffer.data());

        FILE *fp = fopen(fpath, "rb");
        fseek(fp, 0, SEEK_END);
        auto sz = ftell(fp);
        rewind(fp);
        // char *buf = (char *)malloc(sz * sizeof(char));
        std::vector<char> buf(sz);
        size_t bytesRead = fread(&buf[0], sizeof(char), sz, fp);
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