#pragma once
#include "engine/remap_table.h"
#include <nlohmann/json.hpp>

// nlohmann::json finds these via argument-dependent lookup whenever a
// PendingChange (or a container of them) is converted to/from json - that's
// what lets RemapTable::ToJson/FromJson and ProfileStore::Save/Load just
// hand a std::vector<PendingChange> straight to nlohmann::json without any
// manual field-by-field (de)serialization code.
inline void to_json(nlohmann::json& j, const PendingChange& p) {
    j = nlohmann::json{
        {"key_id", p.key_id},
        {"to_label", p.to_label},
        {"to_scan_code", p.to_scan_code},
        {"to_extended", p.to_extended},
        {"disabled", p.disabled}
    };
}

inline void from_json(const nlohmann::json& j, PendingChange& p) {
    p.key_id = j.value("key_id", std::string());
    p.to_label = j.value("to_label", std::string());
    p.to_scan_code = j.value("to_scan_code", static_cast<uint16_t>(0));
    p.to_extended = j.value("to_extended", false);
    p.disabled = j.value("disabled", false);
}
