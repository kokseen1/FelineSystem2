#include <utils.hpp>

#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>
#include <string.h>

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

#ifdef __EMSCRIPTEN__

    std::string getCookie(const std::string &name)
    {
        return std::string(emscripten_run_script_string(
            ("(function getCookie(name) {\
        var nameEQ = name + '=';\
        var ca = document.cookie.split(';');\
        for(var i=0;i < ca.length;i++) {\
            var c = ca[i];\
            while (c.charAt(0) == ' ') c = c.substring(1, c.length);\
            if (c.indexOf(nameEQ) == 0) return c.substring(nameEQ.length, c.length);\
        }\
        return '';\
    })('" + name +
             "');")
                .c_str()));
    }
#endif

}
