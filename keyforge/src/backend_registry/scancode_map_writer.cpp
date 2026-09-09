#include "backend_registry/scancode_map_writer.h"
#ifdef _WIN32
#include <windows.h>
#endif
#include <algorithm>

std::vector<uint8_t> ScancodeMapWriter::BuildBlob(
    const std::vector<RemapEntry>& entries) {
    std::vector<uint8_t> blob;
    blob.resize(8, 0);

    uint32_t count = static_cast<uint32_t>(entries.size()) + 1;
    blob.push_back(static_cast<uint8_t>(count & 0xFF));
    blob.push_back(static_cast<uint8_t>((count >> 8) & 0xFF));
    blob.push_back(0);
    blob.push_back(0);

    for (const auto& e : entries) {
        uint16_t new_code = e.disabled
            ? 0
            : MakeFullScanCode(static_cast<uint8_t>(e.to_scan_code), e.to_extended);
        uint16_t orig_code = MakeFullScanCode(
            static_cast<uint8_t>(e.from_scan_code), e.from_extended);

        blob.push_back(static_cast<uint8_t>(new_code & 0xFF));
        blob.push_back(static_cast<uint8_t>((new_code >> 8) & 0xFF));
        blob.push_back(static_cast<uint8_t>(orig_code & 0xFF));
        blob.push_back(static_cast<uint8_t>((orig_code >> 8) & 0xFF));
    }

    blob.push_back(0);
    blob.push_back(0);
    blob.push_back(0);
    blob.push_back(0);

    return blob;
}

#ifdef _WIN32
bool ScancodeMapWriter::WriteToRegistry(const std::vector<uint8_t>& blob) {
    HKEY hKey;
    LONG result = RegOpenKeyExW(
        HKEY_LOCAL_MACHINE,
        L"SYSTEM\\CurrentControlSet\\Control\\Keyboard Layout",
        0, KEY_SET_VALUE, &hKey);
    if (result != ERROR_SUCCESS) return false;

    result = RegSetValueExW(
        hKey, L"Scancode Map", 0, REG_BINARY,
        blob.data(), static_cast<DWORD>(blob.size()));
    RegCloseKey(hKey);
    return result == ERROR_SUCCESS;
}

bool ScancodeMapWriter::ReadFromRegistry(std::vector<uint8_t>& blob) {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
        L"SYSTEM\\CurrentControlSet\\Control\\Keyboard Layout",
        0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        return false;
    }
    DWORD size = 0;
    if (RegQueryValueExW(hKey, L"Scancode Map", nullptr, nullptr,
        nullptr, &size) != ERROR_SUCCESS || size == 0) {
        RegCloseKey(hKey);
        return false;
    }
    blob.resize(size);
    LONG result = RegQueryValueExW(hKey, L"Scancode Map", nullptr, nullptr,
        blob.data(), &size);
    RegCloseKey(hKey);
    return result == ERROR_SUCCESS;
}

bool ScancodeMapWriter::ClearRegistry() {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
        L"SYSTEM\\CurrentControlSet\\Control\\Keyboard Layout",
        0, KEY_SET_VALUE, &hKey) != ERROR_SUCCESS) {
        return false;
    }
    LONG result = RegDeleteValueW(hKey, L"Scancode Map");
    RegCloseKey(hKey);
    return result == ERROR_SUCCESS || result == ERROR_FILE_NOT_FOUND;
}

bool ScancodeMapWriter::NeedsElevation() {
    HKEY hKey;
    LONG result = RegOpenKeyExW(
        HKEY_LOCAL_MACHINE,
        L"SYSTEM\\CurrentControlSet\\Control\\Keyboard Layout",
        0, KEY_SET_VALUE, &hKey);
    if (result == ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return false;
    }
    return result == ERROR_ACCESS_DENIED;
}
#endif  // _WIN32
