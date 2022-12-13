#pragma once

#include <asmodean.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include <stdio.h>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

#define SAVEDATA_FILENAME "savedata.json"

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

            // Sanitize string
            auto str = ss.str();
            std::replace(str.begin() + 13, str.end() - 3, '`', '\'');

            emscripten_run_script(str.c_str());
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

#ifdef __EMSCRIPTEN__
    std::string getLocalStorage(const std::string &);

    void setLocalStorage(const std::string &, const std::string &);

    std::string getCookie(const std::string &);
#endif

    void save(const std::string &, const json &);

    json load(const std::string &);

    namespace detail
    {
        // Template function to initialize array with default values
        // by making use of comma operator and index_sequence
        template <typename T, std::size_t... Is>
        constexpr std::array<T, sizeof...(Is)>
        create_array(T value, std::index_sequence<Is...>)
        {
            // cast Is to void to remove the warning: unused value
            return {(static_cast<void>(Is), value)...};
        }
    }

    // Dynamically create an array with initialized objects using non-default constructor
    template <std::size_t N, typename T>
    constexpr std::array<T, N> create_array(const T &value)
    {
        return detail::create_array(value, std::make_index_sequence<N>());
    }

    const std::vector<std::string> getAssetArgs(const std::string &);
}
