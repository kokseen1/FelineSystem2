#pragma once

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/fetch.h>
#endif

#include <asmodean.h>
#include <utils.hpp>
#include <blowfish.h>

#include <unordered_map>
#include <vector>
#include <string>
#include <sstream>

#define FFAP_CB(X) void (TClass::*X)(byte *, size_t, TUserdata)

typedef struct
{
    uint32 Offset;
    uint32 Length;
    unsigned char Index;
} KifDbEntry;

typedef struct
{
    std::string Filename;
    unsigned char IsEncrypted;
    byte FileKey[4];
} KifTableEntry;

class SceneManager;

class FileManager
{
public:
    SceneManager *sceneManager = NULL;

    FileManager(const char *);

    std::vector<byte> readFile(const std::string, uint64_t = -1, uint64_t = -1);

    static inline bool ends_with(std::string const &value, std::string const &ending)
    {
        if (ending.size() > value.size())
            return false;
        return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
    }

    template <typename TClass, typename TUserdata>
    void fetchFileAndProcess(std::string fpath, TClass *classobj, FFAP_CB(cb), TUserdata userdata)
    {
        typedef FFAP_CB(TCallback);
        LOG << "Fetch: " << fpath;

        uint64_t offset = -1;
        uint64_t length = -1;

        auto got = kifDb.find(fpath);
        if (got != kifDb.end())
        {
            fpath = ASSETS + kifTable[got->second.Index].Filename;
            std::cout << fpath << std::endl;
            offset = got->second.Offset;
            length = got->second.Length;
        }

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

        const char *headers[] = {"Range", NULL, NULL};
        std::string range;
        if (offset != -1 && length != -1)
        {
            std::ostringstream os;
            // Subtract 1 as last byte should not be included
            os << "bytes=" << offset << "-" << offset + length - 1;
            std::cout << os.str() << std::endl;
            range = os.str();
            headers[1] = range.c_str();
            // const char *headers[] = {"Range", os.str().c_str(), NULL};
            attr.requestHeaders = headers;
        }

        attr.userData = new Arg{
            fpath,
            classobj,
            cb,
            userdata};
        attr.onsuccess = [](emscripten_fetch_t *fetch)
        {
            LOG << "FETCHed " << fetch->numBytes;
            auto buf = reinterpret_cast<const byte *>(fetch->data);
            std::vector<byte> bufVec(buf, buf + fetch->numBytes);

            // Retrieve arguments
            Arg *a = reinterpret_cast<Arg *>(fetch->userData);
            TClass *classobj = a->classobj;
            TCallback cb = a->cb;
            TUserdata userdata = a->userdata;
            std::string fpath = a->fpath;
#else

        // Handle local file read
        auto bufVec = readFile(fpath, offset, length);
        if (bufVec.empty())
        {
            LOG << "Could not read file " << fpath;
            return;
        }

#endif
            if (ends_with(fpath, ".int"))
            {
                LOG << "Decode: " << fpath;
                LOG << "Size: " << bufVec.size();
                Blowfish bf;
                // TODO: Check if index valid
                // TODO: Check if IsEncrypted
                auto key = kifTable[got->second.Index].FileKey;
                bf.SetKey(key, 4);
                bf.Decrypt(bufVec.data(), bufVec.data(), bufVec.size() & ~7);
            }

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

private:
    std::unordered_map<std::string, KifDbEntry> kifDb;
    std::vector<KifTableEntry> kifTable;

    void parseKifDb(byte *, size_t, int);
};