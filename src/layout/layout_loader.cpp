#include "layout/layout_loader.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#include "resource.h"
#endif

using nlohmann::json;

namespace {

// Parses one entry of the "keys" array into a KeyCell, matching the schema
// documented in the build plan (id/label/scan_code/extended/x/y/w/h).
// scan_code is a hex string like "0x1E". A handful of layouts also use the
// sentinel "HARDWARE_FN_NOT_OS_VISIBLE" for a hardware Fn key that never
// reaches the OS at all (see build plan Non-Goals) - any scan_code string
// that isn't a "0x..." hex value is treated as that case: the key is kept
// (so it still renders) but flagged hardware_only so it can't be remapped.
KeyCell ParseKeyObject(const json& j) {
    KeyCell k;
    k.id = j.value("id", std::string());
    k.label = j.value("label", std::string());

    std::string sc = j.value("scan_code", std::string());
    if (!sc.empty()) {
        if (sc.size() >= 2 && sc[0] == '0' && (sc[1] == 'x' || sc[1] == 'X')) {
            try {
                k.scan_code = static_cast<uint16_t>(std::stoul(sc, nullptr, 16));
            } catch (const std::exception&) {
                k.scan_code = 0;
                k.hardware_only = true;
            }
        } else {
            k.scan_code = 0;
            k.hardware_only = true;
        }
    }

    k.extended = j.value("extended", false);
    k.x = j.value("x", 0.0f);
    k.y = j.value("y", 0);
    k.w = j.value("w", 1.0f);
    k.h = j.value("h", 1.0f);

    if (k.scan_code == 0 && !k.hardware_only) {
        k.hardware_only = true;
    }
    return k;
}

}  // namespace

bool LayoutLoader::LoadFromJson(const std::string& json_text, Layout& out_layout) {
    try {
        auto j = json::parse(json_text, nullptr, /*allow_exceptions=*/false);
        if (j.is_discarded() || !j.is_object()) return false;

        out_layout.layout_id = j.value("layout_id", std::string());
        out_layout.display_name = j.value("display_name", std::string());
        out_layout.key_count = j.value("key_count", 0);
        out_layout.unit_px = j.value("unit_px", 54);
        if (out_layout.unit_px <= 0) out_layout.unit_px = 54;
        out_layout.rows = j.value("rows", 0);

        out_layout.keys.clear();
        auto keys_j = j.value("keys", json::array());
        if (!keys_j.is_array()) return false;

        for (const auto& key_j : keys_j) {
            if (!key_j.is_object()) continue;
            out_layout.keys.push_back(ParseKeyObject(key_j));
        }
        return !out_layout.keys.empty();
    } catch (const json::exception&) {
        return false;
    }
}

bool LayoutLoader::LoadFromFile(const std::string& path, Layout& out_layout) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return false;

    std::ostringstream ss;
    ss << file.rdbuf();
    return LoadFromJson(ss.str(), out_layout);
}

#ifdef _WIN32
namespace {
int ResourceIdForLayout(const std::string& layout_id) {
    if (layout_id == "65_percent_ansi") return IDR_LAYOUT_65_PERCENT_ANSI;
    if (layout_id == "sixty_percent") return IDR_LAYOUT_60_PERCENT;
    if (layout_id == "forty_percent") return IDR_LAYOUT_40_PERCENT;
    if (layout_id == "seventy_five_percent") return IDR_LAYOUT_75_PERCENT;
    if (layout_id == "tkl") return IDR_LAYOUT_TKL;
    if (layout_id == "layout_1800_compact") return IDR_LAYOUT_1800_COMPACT;
    if (layout_id == "full_size_ansi") return IDR_LAYOUT_FULL_SIZE_ANSI;
    return 0;
}
}  // namespace

bool LayoutLoader::LoadBuiltIn(const std::string& layout_id, Layout& out_layout) {
    const int resource_id = ResourceIdForLayout(layout_id);
    if (resource_id == 0) return false;

    HMODULE module = GetModuleHandleW(nullptr);
    HRSRC resource = FindResourceW(module, MAKEINTRESOURCEW(resource_id), RT_RCDATA);
    if (!resource) return false;
    HGLOBAL loaded = LoadResource(module, resource);
    const void* bytes = loaded ? LockResource(loaded) : nullptr;
    const DWORD size = SizeofResource(module, resource);
    if (!bytes || size == 0) return false;
    return LoadFromJson(std::string(static_cast<const char*>(bytes), size), out_layout);
}

std::vector<std::string> LayoutLoader::GetLayoutFiles(const std::string& dir) {
    std::vector<std::string> files;
    std::string pattern = dir + "\\*.json";

    WIN32_FIND_DATAA find_data;
    HANDLE h = FindFirstFileA(pattern.c_str(), &find_data);
    if (h == INVALID_HANDLE_VALUE) return files;

    do {
        if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            std::string name = find_data.cFileName;
            size_t dot = name.rfind(".json");
            if (dot != std::string::npos && dot == name.size() - 5) {
                files.push_back(name.substr(0, dot));
            }
        }
    } while (FindNextFileA(h, &find_data));

    FindClose(h);
    return files;
}
#endif
