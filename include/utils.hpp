#pragma once

#include <asmodean.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include <stdio.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#define LOG Utils::Log()

#define LOGGING_ENABLE

namespace Utils
{
    class Log
    {
        std::stringstream ss;

    public:
        Log()
        {
#ifndef LOGGING_ENABLE
            return;
#endif
#ifdef __EMSCRIPTEN__
            ss << "console.log(`";
#else
            ss << "[LOG] ";
#endif
        }

        template <class T>
        Log &operator<<(const T &v)
        {
            ss << v;
            return *this;
        }

        ~Log()
        {
#ifndef LOGGING_ENABLE
            return;
#endif
#ifdef __EMSCRIPTEN__
            ss << "`)";
            emscripten_run_script(ss.str().c_str());
#else
            std::cout << ss.str() << std::endl;
#endif
        }
    };

    inline std::string zeroPad(const std::string str, const size_t len)
    {
        return std::string(len - std::min(len, str.length()), '0') + str;
    }

    std::vector<byte> zlibUncompress(uint32, byte *, uint32 &);

    inline void lowercase(std::string &s)
    {
        for (auto &c : s)
            c = tolower(c);
    }
}
