#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include <stdio.h>
#include <asmodean.h>

#include <iostream>
#include <string>
#include <vector>

#define FFAP_CB(X) void (TClass::*X)(byte *, size_t, TUserdata)

namespace Utils
{
    std::vector<byte> zlibUncompress(uint32, byte *, uint32 &);

    std::vector<byte> readFile(const std::string);

    // Fetch a file and pass the buffer to a callback
    template <typename TClass, typename TUserdata>
    void fetchFileAndProcess(const std::string fpath, TClass *obj, FFAP_CB(cb), TUserdata ud)
    {
        typedef FFAP_CB(TCallback);
        std::cout << "Fetching file " << fpath << std::endl;
#ifdef __EMSCRIPTEN__
        // Struct to store object and callback
        typedef struct
        {
            TClass *obj;
            TCallback cb;
            const std::string fpath;
            TUserdata ud;
        } Arg;

        auto *a = new Arg{obj, cb, fpath, ud};

        emscripten_async_wget_data(
            fpath.c_str(), a,
            [](void *arg, void *buf, int sz)
            {
                // Retrieve arguments
                auto *a = reinterpret_cast<Arg *>(arg);
                auto *obj = a->obj;
                auto cb = a->cb;
                auto ud = a->ud;

                // Call callback function
                (obj->*cb)(static_cast<byte *>(buf), sz, ud);

                delete arg;
            },
            [](void *arg)
            {
                auto *a = reinterpret_cast<Arg *>(arg);
                printf("Could not fetch %s\n", a->fpath.c_str());
            });
#else
        auto buf = readFile(fpath);
        if (buf.empty())
        {
            std::cout << "Could not read file " << fpath << std::endl;
            return;
        }
        (obj->*cb)(buf.data(), buf.size(), ud);
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