#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include <vector>
#include <asmodean.h>

namespace Utils
{
    std::vector<byte> zlibUncompress(uint32, byte *, uint32);

    std::vector<byte> readFile(const char *);

    template <typename T, typename C>
    // typedef void (C::*ProcessFileCallback)(byte *, size_t);
    void processFile(const char *fpath, T obj, C cb)
    {
#ifdef __EMSCRIPTEN__
        // Struct to store object and callback
        typedef struct
        {
            T obj;
            C cb;
        } Arg;

        auto *a = new Arg{obj, cb};

        emscripten_async_wget_data(
            fpath, a,
            [](void *arg, void *buf, int sz)
            {
                // Retrieve arguments
                auto *a = reinterpret_cast<Arg *>(arg);
                auto *obj = a->obj;
                auto cb = a->cb;

                // Call callback function
                (obj->*cb)(static_cast<byte *>(buf), sz);

                delete arg;
            },
            [](void *)
            {
                printf("emscripten_async_wget_data Error\n");
            });
#else
        auto buf = readFile(fpath);
        (obj->*cb)(buf.data(), buf.size());
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