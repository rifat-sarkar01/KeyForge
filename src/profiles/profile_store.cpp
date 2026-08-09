#include "profiles/profile_store.h"
#include "engine/remap_json.h"
#include <fstream>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#endif

using nlohmann::json;

#ifdef _WIN32
ProfileStore::ProfileStore() {
    char appdata[MAX_PATH] = {0};
    if (SHGetFolderPathA(nullptr, CSIDL_APPDATA, nullptr, 0, appdata) == S_OK) {
        m_profiles_dir = std::string(appdata) + "\\KeyForge\\profiles";
    }
    EnsureDirectory();
}

void ProfileStore::EnsureDirectory() {
    if (m_profiles_dir.empty()) return;
    // Create each path segment - CreateDirectoryA doesn't create
    // intermediate directories on its own.
    std::string accum;
    size_t pos = 0;
    while (pos < m_profiles_dir.size()) {
        size_t next = m_profiles_dir.find('\\', pos);
        if (next == std::string::npos) next = m_profiles_dir.size();
        accum = m_profiles_dir.substr(0, next);
        if (!accum.empty()) {
            CreateDirectoryA(accum.c_str(), nullptr);
        }
        pos = next + 1;
    }
}

bool ProfileStore::Delete(const std::string& name) {
    if (m_profiles_dir.empty()) return false;
    std::string path = m_profiles_dir + "\\" + name + ".json";
    return DeleteFileA(path.c_str()) != 0;
}

std::vector<std::string> ProfileStore::ListProfiles() {
    std::vector<std::string> names;
    if (m_profiles_dir.empty()) return names;

    std::string pattern = m_profiles_dir + "\\*.json";
    WIN32_FIND_DATAA find_data;
    HANDLE h = FindFirstFileA(pattern.c_str(), &find_data);
    if (h == INVALID_HANDLE_VALUE) return names;

    do {
        if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            std::string name = find_data.cFileName;
            size_t dot = name.rfind(".json");
            if (dot != std::string::npos && dot == name.size() - 5) {
                names.push_back(name.substr(0, dot));
            }
        }
    } while (FindNextFileA(h, &find_data));

    FindClose(h);
    return names;
}
#endif  // _WIN32

Profile ProfileStore::CreateDefault() const {
    Profile p;
    p.name = "Default";
    p.layout_id = "65_percent_ansi";
    return p;
}

bool ProfileStore::Save(const Profile& profile) {
    if (m_profiles_dir.empty() || profile.name.empty()) return false;
    std::string path = m_profiles_dir + "\\" + profile.name + ".json";

    json j;
    j["profile_name"] = profile.name;
    j["layout_id"] = profile.layout_id;
    j["remaps"] = profile.remaps;

    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) return false;
    file << j.dump(2);
    return file.good();
}

bool ProfileStore::Load(const std::string& name, Profile& out_profile) {
    if (m_profiles_dir.empty()) return false;
    std::string path = m_profiles_dir + "\\" + name + ".json";

    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return false;

    std::ostringstream ss;
    ss << file.rdbuf();

    auto j = json::parse(ss.str(), nullptr, /*allow_exceptions=*/false);
    if (j.is_discarded() || !j.is_object()) return false;

    try {
        out_profile.name = j.value("profile_name", name);
        out_profile.layout_id = j.value("layout_id", std::string());
        out_profile.remaps.clear();
        if (j.contains("remaps") && j["remaps"].is_array()) {
            out_profile.remaps = j["remaps"].get<std::vector<PendingChange>>();
        }
    } catch (const json::exception&) {
        return false;
    }
    return true;
}
