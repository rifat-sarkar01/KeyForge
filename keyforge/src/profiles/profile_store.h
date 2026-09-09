#pragma once
#include "engine/remap_table.h"
#include "layout/layout_types.h"
#include <string>
#include <vector>

struct Profile {
    std::string name;
    std::string layout_id;
    std::vector<PendingChange> remaps;
};

class ProfileStore {
public:
    ProfileStore();
    bool Save(const Profile& profile);
    bool Load(const std::string& name, Profile& out_profile);
    bool Delete(const std::string& name);
    std::vector<std::string> ListProfiles();
    std::string GetProfilesDir() const { return m_profiles_dir; }
    Profile CreateDefault() const;

private:
    std::string m_profiles_dir;
    void EnsureDirectory();
};
