#pragma once

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/fetch.h>
#endif

#include <asmodean.h>
#include <utils.hpp>

#include <vector>
#include <string>

#define FFAP_CB(X) void (TClass::*X)(byte *, size_t, TUserdata)

class FileManager
{
public:
    std::vector<byte> readFile(const std::string, long = -1, long = -1);

    template <typename TClass, typename TUserdata>
    void fetchFileAndProcess(const std::string &fpath, TClass *classobj, FFAP_CB(cb), TUserdata userdata)
    {
        typedef FFAP_CB(TCallback);
        LOG << "Fetch: " << fpath;

#ifdef __EMSCRIPTEN__

        // Define struct to pass to callback as arg
        typedef struct
        {
            const std::string fpath;
            TClass *classobj;
            TCallback cb;
            TUserdata userdata;
        } Arg;

        emscripten_fetch_attr_t attr;
        emscripten_fetch_attr_init(&attr);
        strcpy(attr.requestMethod, "GET");
        attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
        // const char *headers[] = {"Range", "bytes=10-20", NULL};
        // attr.requestHeaders = headers;
        attr.userData = new Arg{
            fpath,
            classobj,
            cb,
            userdata};
        attr.onsuccess = [](emscripten_fetch_t *fetch)
        {
            auto buf = reinterpret_cast<const byte *>(fetch->data);
            std::vector<byte> bufVec(buf, buf + fetch->numBytes);

            // Retrieve arguments
            Arg *a = reinterpret_cast<Arg *>(fetch->userData);
            TClass *classobj = a->classobj;
            TCallback cb = a->cb;
            TUserdata userdata = a->userdata;
#else

        // Handle local file read
        auto bufVec = readFile(fpath);
        if (bufVec.empty())
        {
            LOG << "Could not read file " << fpath;
            return;
        }

#endif
            // Call callback function
            (classobj->*cb)(bufVec.data(), bufVec.size(), userdata);

#ifdef __EMSCRIPTEN__

            delete fetch->userData;
        };
        attr.onerror = [](emscripten_fetch_t *fetch)
        {
            std::cout << fetch->statusText << ": " << fetch->url << std::endl;
            delete fetch->userData;
        };
        emscripten_fetch(&attr, fpath.c_str());
#endif
    }

    // Fetch a file and pass the buffer to a callback
    template <typename TClass, typename TUserdata>
    void fetchFileAndProcessOld(const std::string &fpath, TClass *classobj, FFAP_CB(cb), TUserdata userdata)
    {
        typedef FFAP_CB(TCallback);
        LOG << "Fetch: " << fpath;

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
            LOG << "Could not read file " << fpath;
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
                LOG << "Could not fetch file " << fpath;
                delete arg;
            });
#endif
    }
};