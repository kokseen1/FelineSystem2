#pragma once

#include <utils.hpp>
#include <asmodean.h>
#include <blowfish.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/fetch.h>
// Large file support
#define fseek64 fseeko
#define ftell64 ftello
#else
#define fseek64 _fseeki64
#define ftell64 _ftelli64
#endif

#include <unordered_map>
#include <vector>
#include <string>
#include <sstream>

// Macro for function signature of callbacks passed to FFAP function
#define FFAP_CB(X) void (TClass::*X)(byte *, size_t, TUserdata)

// Custom database of asset names and offsets
#define KIF_DB ASSETS "kif.fs2"

#define LOWERCASE_ASSETS

typedef struct
{
    uint32 Offset;
    uint32 Length;
    unsigned char Index;
} KifDbEntry;

typedef struct
{
    const std::string Filename;
    const unsigned char IsEncrypted;
    byte FileKey[4];
} KifTableEntry;

// Forward declaration
class SceneManager;

class FileManager
{
public:
    FileManager();

    void init(SceneManager *);

    // Find asset in KIF DB and fetch it and pass data to callback
    template <typename TClass, typename TUserdata>
    void fetchAssetAndProcess(const std::string &fname, TClass *classobj, FFAP_CB(cb), TUserdata userdata)
    {
#ifdef LOWERCASE_ASSETS
        Utils::lowercase(const_cast<std::string &>(fname));
#endif
        auto got = kifDb.find(fname);
        if (got != kifDb.end())
        {
            const auto &kte = kifTable[got->second.Index];
            auto offset = got->second.Offset;
            auto length = got->second.Length;

            typedef FFAP_CB(TCallback);

            // Struct containing original callback data to pass to decryption function callback
            typedef struct
            {
                KifTableEntry kte;
                TClass *classobj;
                TCallback cb;
                TUserdata userdata;
            } A;

            const A a = {
                kte,
                classobj,
                cb,
                userdata};

            fetchFileAndProcess(ASSETS + kte.Filename, this, &FileManager::decryptKifAndProcess<A>, a, offset, length);
        }
    }

    // Multi-platform function to fetch a file and pass the contents to a callback
    // Supports optional offset and length arguments
    template <typename TClass, typename TUserdata>
    void fetchFileAndProcess(const std::string &fpath, TClass *classobj, FFAP_CB(cb), const TUserdata &userdata, uint64_t offset = 0, uint64_t length = 0)
    {
        typedef FFAP_CB(TCallback);

        // LOG << "Fetch: " << fpath;

#ifdef __EMSCRIPTEN__

        emscripten_fetch_attr_t attr;
        emscripten_fetch_attr_init(&attr);
        strcpy(attr.requestMethod, "GET");
        attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;

        // Define struct to hold additional callback data in addition to userdata
        typedef struct
        {
            TClass *classobj;
            TCallback cb;
            TUserdata userdata;
        } Misc;

        attr.userData = new Misc{
            classobj,
            cb,
            userdata};

        // Handle optional byte range request
        const char *headers[] = {"Range", NULL, NULL};
        std::string range;
        if (length != 0)
        {
            std::ostringstream os;
            // Subtract 1 as last byte should not be included
            os << "bytes=" << offset << "-" << offset + length - 1;
            range = os.str();
            headers[1] = range.c_str();
            attr.requestHeaders = headers;
        }

        // Success callback
        attr.onsuccess = [](emscripten_fetch_t *fetch)
        {
            LOG << "Fetched: " << fetch->url << " Size: " << fetch->numBytes;
            if (fetch->numBytes == 0)
            {
                LOG << "Fetch failed!";
                delete fetch->userData;
                emscripten_fetch_close(fetch);
                return;
            }

            auto buf = reinterpret_cast<const byte *>(fetch->data);
            std::vector<byte> bufVec(buf, buf + fetch->numBytes);

            // Retrieve arguments
            auto a = reinterpret_cast<Misc *>(fetch->userData);
            TClass *classobj = a->classobj;
            TCallback cb = a->cb;
            TUserdata userdata = a->userdata;

#else

        // Handle local file read
        auto bufVec = readFile(fpath, offset, length);
        if (bufVec.empty())
        {
            LOG << "Could not read local file " << fpath;
            return;
        }

#endif

            // Call callback function
            (classobj->*cb)(bufVec.data(), bufVec.size(), userdata);

#ifdef __EMSCRIPTEN__

            delete fetch->userData;
            emscripten_fetch_close(fetch);
        };

        attr.onerror = [](emscripten_fetch_t *fetch)
        {
            LOG << fetch->statusText << ": " << fetch->url;
            delete fetch->userData;
            emscripten_fetch_close(fetch);
        };

        emscripten_fetch(&attr, fpath.c_str());
#endif
    }

    // Return true if asset is in database
    inline bool inDB(const std::string &name)
    {
        return kifDb.find(name) != kifDb.end();
    }

private:
    SceneManager *sceneManager = NULL;

    // Map of each asset to their respective offset and length and archive index in the KIF table
    std::unordered_map<std::string, KifDbEntry> kifDb;

    // Vector of KIF archives along with their decryption keys
    std::vector<KifTableEntry> kifTable;

    void parseKifDb(byte *, size_t, int);

    // Post-fetch decryption function for KIF assets
    // Passes the decrypted asset to the original callback
    template <typename A>
    void decryptKifAndProcess(byte *data, size_t sz, const A a)
    {
        if (a.kte.IsEncrypted == '\x01')
        {
            // Blowfish decryption
            Blowfish bf;
            bf.SetKey(a.kte.FileKey, 4);
            bf.Decrypt(data, data, sz & ~7);
        }

        // Call the original callback function
        (a.classobj->*a.cb)(data, sz, a.userdata);
    }

    std::vector<byte> readFile(const std::string &, uint64_t = 0, uint64_t = 0);
};
