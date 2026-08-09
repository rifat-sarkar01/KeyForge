#pragma once
#include "engine/scancode_utils.h"
#include "layout/layout_types.h"
#include <array>
#include <vector>
#include <string>
#include <functional>

// A single active remap: physical key `key_id` now sends the scan code
// (to_scan_code, to_extended), or is disabled entirely. `to_label` is purely
// cosmetic - the human-readable name of the remap target (e.g. "Left Ctrl" or
// "Volume Up") - so the UI can show what a key was mapped to without having
// to re-derive it. It has no effect on the engine, which only ever acts on
// the scan code fields.
struct PendingChange {
    std::string key_id;
    std::string to_label;
    uint16_t to_scan_code = 0;
    bool to_extended = false;
    bool disabled = false;
};

class RemapTable {
public:
    void AddRemap(const std::string& key_id, const std::string& to_label,
                  uint16_t to_scan_code, bool to_extended, bool disabled);
    void RemoveRemap(const std::string& key_id);
    void Clear();
    bool HasChanges() const { return !m_pending.empty(); }
    const std::vector<PendingChange>& GetPending() const { return m_pending; }

    std::vector<RemapEntry> BuildEntries(
        const Layout& layout) const;

    std::array<uint16_t, 512> GetLookupTable(
        const Layout& layout) const;

    std::string ToJson() const;
    void FromJson(const std::string& json, const Layout& layout);

private:
    std::vector<PendingChange> m_pending;
};
