#include <windows.h>
#include <shellapi.h>
#include <string>
#include <vector>
#include "ui/main_window.h"
#include "profiles/profile_store.h"
#include "layout/layout_loader.h"
#include "engine/remap_table.h"
#include "backend_registry/scancode_map_writer.h"

namespace {
int HexValue(wchar_t c) {
    if (c >= L'0' && c <= L'9') return c - L'0';
    if (c >= L'a' && c <= L'f') return c - L'a' + 10;
    if (c >= L'A' && c <= L'F') return c - L'A' + 10;
    return -1;
}

bool ParseScancodeMapBlob(const wchar_t* hex, std::vector<uint8_t>& blob) {
    if (!hex) return false;
    const size_t length = wcslen(hex);
    if (length < 32 || length % 2 != 0) return false;

    blob.clear();
    blob.reserve(length / 2);
    for (size_t i = 0; i < length; i += 2) {
        const int high = HexValue(hex[i]);
        const int low = HexValue(hex[i + 1]);
        if (high < 0 || low < 0) return false;
        blob.push_back(static_cast<uint8_t>((high << 4) | low));
    }

    // A keyboard Scancode Map is an 8-byte header, a 32-bit entry count,
    // (count - 1) mappings, and a 4-byte terminator.
    const uint32_t count = static_cast<uint32_t>(blob[8]) |
        (static_cast<uint32_t>(blob[9]) << 8) |
        (static_cast<uint32_t>(blob[10]) << 16) |
        (static_cast<uint32_t>(blob[11]) << 24);
    const size_t expected_size = 12ull + 4ull * count;
    return count >= 1 && expected_size == blob.size();
}

std::string ToUtf8(const wchar_t* value) {
    if (!value || !*value) return {};
    const int size = WideCharToMultiByte(CP_UTF8, 0, value, -1, nullptr, 0, nullptr, nullptr);
    if (size <= 1) return {};
    std::string result(static_cast<size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value, -1, result.data(), size, nullptr, nullptr);
    result.pop_back();
    return result;
}

int ApplyPermanentBlob(const wchar_t* encoded_blob) {
    std::vector<uint8_t> blob;
    if (!ParseScancodeMapBlob(encoded_blob, blob)) {
        MessageBoxW(nullptr, L"KeyForge received an invalid permanent remap.",
                    L"KeyForge", MB_OK | MB_ICONERROR);
        return 1;
    }
    if (!ScancodeMapWriter::WriteToRegistry(blob)) {
        MessageBoxW(nullptr, L"KeyForge could not update the keyboard registry map.",
                    L"KeyForge", MB_OK | MB_ICONERROR);
        return 1;
    }
    MessageBoxW(nullptr,
        L"Permanent remaps were saved. Restart Windows for them to take effect.",
        L"KeyForge", MB_OK | MB_ICONINFORMATION);
    return 0;
}

int ApplyPermanentProfile(const wchar_t* profile_name) {
    Profile profile;
    ProfileStore store;
    if (!store.Load(ToUtf8(profile_name), profile)) {
        MessageBoxW(nullptr, L"The selected profile could not be loaded.",
                    L"KeyForge", MB_OK | MB_ICONERROR);
        return 1;
    }

    Layout layout;
    if (!LayoutLoader::LoadBuiltIn(profile.layout_id, layout)) {
        MessageBoxW(nullptr, L"The profile uses an unavailable keyboard layout.",
                    L"KeyForge", MB_OK | MB_ICONERROR);
        return 1;
    }

    RemapTable table;
    for (const auto& remap : profile.remaps) {
        table.AddRemap(remap.key_id, remap.to_label, remap.to_scan_code,
                       remap.to_extended, remap.disabled);
    }
    if (!ScancodeMapWriter::WriteToRegistry(
            ScancodeMapWriter::BuildBlob(table.BuildEntries(layout)))) {
        MessageBoxW(nullptr, L"KeyForge could not update the keyboard registry map.",
                    L"KeyForge", MB_OK | MB_ICONERROR);
        return 1;
    }
    MessageBoxW(nullptr,
        L"Permanent remaps were saved. Restart Windows for them to take effect.",
        L"KeyForge", MB_OK | MB_ICONINFORMATION);
    return 0;
}

int ClearPermanentProfile() {
    if (!ScancodeMapWriter::ClearRegistry()) {
        MessageBoxW(nullptr, L"KeyForge could not clear the keyboard registry map.",
                    L"KeyForge", MB_OK | MB_ICONERROR);
        return 1;
    }
    MessageBoxW(nullptr,
        L"Permanent remaps were cleared. Restart Windows for the change to take effect.",
        L"KeyForge", MB_OK | MB_ICONINFORMATION);
    return 0;
}
}  // namespace

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int) {
    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argv && argc > 2) {
        for (int i = 1; i < argc; ++i) {
            if (wcscmp(argv[i], L"--apply-permanent") == 0 && i + 1 < argc) {
                const int result = ApplyPermanentProfile(argv[i + 1]);
                LocalFree(argv);
                return result;
            }
            if (wcscmp(argv[i], L"--apply-permanent-blob") == 0 && i + 1 < argc) {
                const int result = ApplyPermanentBlob(argv[i + 1]);
                LocalFree(argv);
                return result;
            }
            if (wcscmp(argv[i], L"--clear-permanent") == 0) {
                const int result = ClearPermanentProfile();
                LocalFree(argv);
                return result;
            }
        }
    }
    if (argv) LocalFree(argv);

    auto& win = MainWindow::Instance();
    if (!win.Create(hInstance)) {
        return 1;
    }
    win.RunMessageLoop();
    return 0;
}
