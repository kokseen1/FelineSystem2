#include <utils.hpp>

#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>
#include <string.h>

#include <fstream>

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
#else
        json j;

        std::ifstream ifs(SAVEDATA_FILENAME);
        if (ifs.is_open())
        {
            try
            {
                j = json::parse(ifs);
            }
            catch (const json::parse_error &e)
            {
                // Invalid savedata file, ignore and create a new one
            }
        }
        j[name] = data;
        std::ofstream ofs(SAVEDATA_FILENAME);
        ofs << j;
#endif
    }

    // Platform specific load
    json load(const std::string &name)
    {
        try
        {
#ifdef __EMSCRIPTEN__
            const std::string &dataRaw = getLocalStorage(name);
            return json::parse(dataRaw);
#else
            std::ifstream ifs(SAVEDATA_FILENAME);
            return json::parse(ifs).at(name);
#endif
        }
        catch (const json::parse_error &e)
        {
            return {};
        }
        catch (const json::out_of_range &e)
        {
            return {};
        }
    }

    // Parse comma separated asset names
    const std::vector<std::string> getAssetArgs(const std::string &asset)
    {
        std::stringstream ss(asset);
        std::vector<std::string> args;

        while (ss.good())
        {
            std::string arg;
            getline(ss, arg, ',');
            args.push_back(arg);
        }

        return args;
    }
}
