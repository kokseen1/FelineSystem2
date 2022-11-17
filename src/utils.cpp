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

    std::string getLocalStorage(const std::string &key)
    {
        return std::string(emscripten_run_script_string(
            ("(function getLocalStorage(key){\
            var value = localStorage.getItem(key);\
            if (value === null) return '';\
            return value;\
            })('" +
             key + "');")
                .c_str()));
    }

    void setLocalStorage(const std::string &key, const std::string &value)
    {
        EM_ASM(
            {
                var k = Module.UTF8ToString($0, $1);
                var v = Module.UTF8ToString($2, $3);
                localStorage.setItem(k, v);
            },
            key.c_str(), key.length(), value.c_str(), value.length());
    }

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

    // Platform specific save
    void save(const std::string &name, const json &data)
    {
#ifdef __EMSCRIPTEN__
        setLocalStorage(name, data.dump());
#endif
    }

    // Platform specific load
    json load(const std::string &name)
    {
#ifdef __EMSCRIPTEN__
        const std::string &dataRaw = getLocalStorage(name);
        if (dataRaw.empty())
        {
            // Non existent save data
            return {};
        }
        return json::parse(dataRaw);
#endif
    }
}
