#include "engine/remap_table.h"
#include "engine/remap_json.h"
#include <algorithm>

void RemapTable::AddRemap(const std::string& key_id,
                           const std::string& to_label,
                           uint16_t to_scan_code, bool to_extended,
                           bool disabled) {
    auto it = std::find_if(m_pending.begin(), m_pending.end(),
        [&](const PendingChange& p) { return p.key_id == key_id; });
    if (it != m_pending.end()) {
        it->to_label = to_label;
        it->to_scan_code = to_scan_code;
        it->to_extended = to_extended;
        it->disabled = disabled;
    } else {
        m_pending.push_back({key_id, to_label, to_scan_code, to_extended, disabled});
    }
}

void RemapTable::RemoveRemap(const std::string& key_id) {
    m_pending.erase(
        std::remove_if(m_pending.begin(), m_pending.end(),
            [&](const PendingChange& p) { return p.key_id == key_id; }),
        m_pending.end());
}

void RemapTable::Clear() {
    m_pending.clear();
}

std::vector<RemapEntry> RemapTable::BuildEntries(const Layout& layout) const {
    std::vector<RemapEntry> entries;
    for (const auto& change : m_pending) {
        auto key_it = std::find_if(layout.keys.begin(), layout.keys.end(),
            [&](const KeyCell& k) { return k.id == change.key_id; });
        if (key_it != layout.keys.end() && !key_it->hardware_only) {
            RemapEntry e;
            e.from_scan_code = key_it->scan_code;
            e.from_extended = key_it->extended;
            e.to_scan_code = change.to_scan_code;
            e.to_extended = change.to_extended;
            e.disabled = change.disabled;
            entries.push_back(e);
        }
    }
    return entries;
}

std::array<uint16_t, 512> RemapTable::GetLookupTable(
    const Layout& layout) const {
    return BuildLookupTable(BuildEntries(layout));
}

std::string RemapTable::ToJson() const {
    return nlohmann::json(m_pending).dump();
}

void RemapTable::FromJson(const std::string& json_text, const Layout& layout) {
    m_pending.clear();
    auto j = nlohmann::json::parse(json_text, nullptr, /*allow_exceptions=*/false);
    if (j.is_discarded() || !j.is_array()) return;

    try {
        auto parsed = j.get<std::vector<PendingChange>>();
        for (auto& change : parsed) {
            // Only keep remaps whose source key still exists on this layout -
            // e.g. a profile saved against a different/older layout revision
            // may reference a key id that no longer exists.
            auto key_it = std::find_if(layout.keys.begin(), layout.keys.end(),
                [&](const KeyCell& k) { return k.id == change.key_id; });
            if (key_it != layout.keys.end()) {
                m_pending.push_back(std::move(change));
            }
        }
    } catch (const nlohmann::json::exception&) {
        m_pending.clear();
    }
}
