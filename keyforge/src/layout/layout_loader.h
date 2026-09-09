#pragma once
#include "layout/layout_types.h"
#include <string>
#include <vector>

class LayoutLoader {
public:
    static bool LoadFromFile(const std::string& path, Layout& out_layout);
    static bool LoadFromJson(const std::string& json, Layout& out_layout);
#ifdef _WIN32
    static bool LoadBuiltIn(const std::string& layout_id, Layout& out_layout);
#endif
    static std::vector<std::string> GetLayoutFiles(const std::string& dir);
};
