#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include <stdio.h>
#include <asmodean.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#define FFAP_CB(X) void (TClass::*X)(byte *, size_t, TUserdata)
#define LOG Utils::Log()

namespace Utils
{
    class Log
    {
        std::stringstream ss;

    public:
        Log()
        {
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
#ifdef __EMSCRIPTEN__
            ss << "`)";
            emscripten_run_script(ss.str().c_str());
#else
            std::cout << ss.str() << std::endl;
#endif
        }
    };

    std::string zeroPad(std::string, size_t);

    std::vector<byte> zlibUncompress(uint32, byte *, uint32 &);

    std::vector<byte> readFile(const std::string);

    // Fetch a file and pass the buffer to a callback
    template <typename TClass, typename TUserdata>
    void fetchFileAndProcess(const std::string &fpath, TClass *classobj, FFAP_CB(cb), TUserdata userdata)
    {
        typedef FFAP_CB(TCallback);
        // std::cout << "Fetching file " << fpath << std::endl;

#ifdef __EMSCRIPTEN__

        // Define struct to pass to callback as arg
        typedef struct
        {
            const std::string fpath;
            TClass *classobj;
            TCallback cb;
            TUserdata userdata;
        } Arg;

        Arg *a = new Arg{
            fpath,
            classobj,
            cb,
            userdata};

        emscripten_async_wget_data(
            fpath.c_str(),
            a,
            [](void *arg, void *buf, int sz)
            {
                std::vector<byte> bufVec(static_cast<byte *>(buf), static_cast<byte *>(buf) + sz);

                // Retrieve arguments
                Arg *a = reinterpret_cast<Arg *>(arg);
                TClass *classobj = a->classobj;
                TCallback cb = a->cb;
                TUserdata userdata = a->userdata;

#else

        // Handle local file read
        auto bufVec = readFile(fpath);
        if (bufVec.empty())
        {
            // std::cout << "Could not read file " << fpath << std::endl;
            return;
        }

#endif

                // Call callback function
                (classobj->*cb)(bufVec.data(), bufVec.size(), userdata);

#ifdef __EMSCRIPTEN__

                delete arg;
            },

            // onError callback
            [](void *arg)
            {
                Arg *a = reinterpret_cast<Arg *>(arg);
                auto fpath = a->fpath;
                // std::cout << "Could not fetch file " << fpath << std::endl;
            });
#endif
    }
}