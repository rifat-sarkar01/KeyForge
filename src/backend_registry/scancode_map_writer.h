#pragma once
#include "engine/scancode_utils.h"
#include <vector>
#include <cstdint>

class ScancodeMapWriter {
public:
    static std::vector<uint8_t> BuildBlob(const std::vector<RemapEntry>& entries);
    static bool WriteToRegistry(const std::vector<uint8_t>& blob);
    static bool ReadFromRegistry(std::vector<uint8_t>& blob);
    static bool ClearRegistry();
    static bool NeedsElevation();
};
