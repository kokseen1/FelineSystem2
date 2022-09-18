#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include <vector>
#include <asmodean.h>

namespace Utils
{
    std::vector<byte> zlibUncompress(uint32, byte *, uint32);

    std::vector<byte> readFile(const char *);

#ifdef __EMSCRIPTEN__
    void readFile(const char *, void *, em_async_wget_onload_func, em_arg_callback_func);

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