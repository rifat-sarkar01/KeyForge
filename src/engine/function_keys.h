#pragma once
#include <cstdint>
#include <string>
#include <vector>

// A "function key" here is a virtual remap target that does not correspond to
// a physical key on the visual keyboard layout - things like Volume Up/Down,
// Play/Pause, or Browser Back. The engine doesn't need to know these are
// "special": they are plain scan codes, exactly like any physical key, so
// they slot into the existing RemapEntry / Scancode-Map machinery unchanged.
//
// All of these are E0-prefixed (extended = true) Set 1 scan codes from the
// Microsoft Natural Multimedia Keyboard's OEM scan code set, which is the
// scan code table Windows itself recognizes for these consumer-control
// functions. Values cross-checked against multiple independent scan code
// references (not just one source), since a wrong byte here would silently
// remap a key to the wrong function.
struct FunctionKeyDef {
    const char* id;       // stable id, used the same way a physical key's id is
    const char* label;    // human-readable label shown in the key picker
    uint8_t scan_code;    // base (non-extended) byte
    bool extended;        // true => E0-prefixed
};

inline const std::vector<FunctionKeyDef>& GetFunctionKeys() {
    static const std::vector<FunctionKeyDef> keys = {
        { "FN_VOLUME_MUTE",      "Volume Mute",        0x20, true },
        { "FN_VOLUME_DOWN",      "Volume Down",         0x2E, true },
        { "FN_VOLUME_UP",        "Volume Up",           0x30, true },
        { "FN_MEDIA_PLAY_PAUSE", "Play / Pause",        0x22, true },
        { "FN_MEDIA_STOP",       "Media Stop",          0x24, true },
        { "FN_MEDIA_NEXT",       "Next Track",          0x19, true },
        { "FN_MEDIA_PREV",       "Previous Track",      0x10, true },
        { "FN_MEDIA_SELECT",     "Media Select",        0x6D, true },
        { "FN_MAIL",             "Launch Mail",         0x6C, true },
        { "FN_CALCULATOR",       "Launch Calculator",   0x21, true },
        { "FN_MY_COMPUTER",      "Launch My Computer",  0x6B, true },
        { "FN_BROWSER_HOME",     "Browser Home",        0x32, true },
        { "FN_BROWSER_SEARCH",   "Browser Search",      0x65, true },
        { "FN_BROWSER_FAVORITES","Browser Favorites",   0x66, true },
        { "FN_BROWSER_BACK",     "Browser Back",        0x6A, true },
        { "FN_BROWSER_FORWARD",  "Browser Forward",     0x69, true },
        { "FN_BROWSER_REFRESH",  "Browser Refresh",     0x67, true },
        { "FN_BROWSER_STOP",     "Browser Stop",        0x68, true },
        { "FN_SLEEP",            "Sleep",               0x5F, true },
    };
    return keys;
}

// Looks up a function key definition by id. Returns nullptr if not found -
// used when re-hydrating a saved profile/remap-table entry that targeted a
// function key, so the UI can show a proper label instead of a raw scan code.
inline const FunctionKeyDef* FindFunctionKeyById(const std::string& id) {
    for (const auto& fk : GetFunctionKeys()) {
        if (id == fk.id) return &fk;
    }
    return nullptr;
}
