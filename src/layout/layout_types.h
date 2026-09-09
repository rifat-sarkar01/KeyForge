#pragma once
#include <string>
#include <vector>
#include <cstdint>

struct KeyCell {
    std::string id;
    std::string label;
    uint16_t scan_code = 0;
    bool extended = false;
    float x = 0.0f;
    int y = 0;
    float w = 1.0f;
    float h = 1.0f;
    bool hardware_only = false;
};

struct Layout {
    std::string layout_id;
    std::string display_name;
    int key_count = 0;
    int unit_px = 54;
    int rows = 0;
    std::vector<KeyCell> keys;
};
