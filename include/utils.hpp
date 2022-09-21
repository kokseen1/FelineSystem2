#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include <string>
#include <vector>
#include <asmodean.h>

#define FFAP_CALLBACK(x) void (T::*x)(byte *, size_t, const std::string &)

// extern std::map<std::string, std::vector<byte>> fileCache;

namespace Utils
{
    std::vector<byte> zlibUncompress(uint32, byte *, uint32);

    std::vector<byte> readFile(const char *);

    // Asynchronously fetch a file and pass the buffer to a callback
    template <typename T>
    void fetchFileAndProcess(const char *fpath, T *obj, FFAP_CALLBACK(cb))
    {
        typedef FFAP_CALLBACK(CbT);
        printf("Processing %s\n", fpath);
#ifdef __EMSCRIPTEN__
        // Retrieve buffer from cache
        // if (fileCache.find(fpath) != fileCache.end())
        // {
        // (obj->*cb)(fileCache[fpath].data(), fileCache[fpath].size());
        // return;
        // }

        // Struct to store object and callback
        typedef struct
        {
            T *obj;
            CbT cb;
            std::string fpath_str;
        } Arg;

        auto *a = new Arg{obj, cb, fpath};

        emscripten_async_wget_data(
            fpath, a,
            [](void *arg, void *buf, int sz)
            {
                // Retrieve arguments
                auto *a = reinterpret_cast<Arg *>(arg);
                auto *obj = a->obj;
                auto cb = a->cb;
                auto fpath = a->fpath_str;

                // Cache buffer as a vector
                // std::vector<byte> buf_vec((static_cast<byte *>(buf)), (static_cast<byte *>(buf) + sz));
                // fileCache.insert({a->fpath_str, buf_vec});

                // Call callback function
                (obj->*cb)(static_cast<byte *>(buf), sz, fpath);

                delete arg;
            },
            [](void *arg)
            {
                auto *a = reinterpret_cast<Arg *>(arg);
                printf("Could not fetch %s\n", a->fpath_str.c_str());
            });
#else
        auto buf = readFile(fpath);
        if (buf.empty())
        {
            printf("Could not read file %s\n", fpath);
            return;
        }
        (obj->*cb)(buf.data(), buf.size(), fpath);
#endif
    }

#ifdef __EMSCRIPTEN__
    static inline const char *emscripten_event_type_to_string(int eventType)
    {
        const char *events[] = {"(invalid)", "(none)", "keypress", "keydown", "keyup", "click", "mousedown", "mouseup", "dblclick", "mousemove", "wheel", "resize",
                                "scroll", "blur", "focus", "focusin", "focusout", "deviceorientation", "devicemotion", "orientationchange", "fullscreenchange", "pointerlockchange",
                                "visibilitychange", "touchstart", "touchend", "touchmove", "touchcancel", "gamepadconnected", "gamepaddisconnected", "beforeunload",
                                "batterychargingchange", "batterylevelchange", "webglcontextlost", "webglcontextrestored", "(invalid)"};
        ++eventType;
        if (eventType < 0)
            eventType = 0;
        if (eventType >= sizeof(events) / sizeof(events[0]))
            eventType = sizeof(events) / sizeof(events[0]) - 1;
        return events[eventType];
    }
#endif
}