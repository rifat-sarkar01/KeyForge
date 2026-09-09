#pragma once
#include <cstdint>
#include <array>
#include <vector>

constexpr uint16_t MakeFullScanCode(uint8_t code, bool extended) {
    return extended ? (0xE000 | code) : code;
}

constexpr uint8_t GetBaseCode(uint16_t full_scan_code) {
    return static_cast<uint8_t>(full_scan_code & 0xFF);
}

constexpr bool IsExtended(uint16_t full_scan_code) {
    return (full_scan_code & 0xFF00) == 0xE000;
}

constexpr uint16_t DisabledScanCode = 0xFFFF;

// Pause/Break is emitted as a multi-byte E1 sequence rather than a normal
// make/break scan code. Keep special virtual-key sources separate from normal
// scan codes so Pause can never be mistaken for Num Lock (0x45).
constexpr uint32_t PauseVirtualKey = 0x13;
constexpr uint32_t CancelVirtualKey = 0x03;
constexpr uint32_t NumLockVirtualKey = 0x90;

enum class HookSourceKind : uint8_t {
    ScanCode,
    Pause,
    Break,
};

constexpr HookSourceKind ClassifyHookSource(uint32_t virtual_key,
                                            bool control_down) {
    if (virtual_key == CancelVirtualKey ||
        (virtual_key == PauseVirtualKey && control_down)) {
        return HookSourceKind::Break;
    }
    return virtual_key == PauseVirtualKey ? HookSourceKind::Pause
                                          : HookSourceKind::ScanCode;
}

constexpr uint16_t SourceScanCodeForHook(uint32_t scan_code, bool extended,
                                         HookSourceKind source_kind) {
    return source_kind == HookSourceKind::Pause
        ? MakeFullScanCode(0xE1, true)
        : MakeFullScanCode(static_cast<uint8_t>(scan_code), extended);
}

// Normal and E0-extended scan codes share a low byte, so they use separate
// 256-entry banks in the live lookup table.
constexpr size_t LookupIndex(uint16_t full_scan_code) {
    return IsExtended(full_scan_code) ? 256u + GetBaseCode(full_scan_code)
                                     : GetBaseCode(full_scan_code);
}

struct RemapEntry {
    uint16_t from_scan_code = 0;
    uint16_t to_scan_code = 0;
    bool from_extended = false;
    bool to_extended = false;
    bool disabled = false;
};

inline std::array<uint16_t, 512> BuildLookupTable(
    const std::vector<RemapEntry>& entries) {
    std::array<uint16_t, 512> table{};
    table.fill(0);
    for (const auto& e : entries) {
        uint16_t from = MakeFullScanCode(
            static_cast<uint8_t>(e.from_scan_code), e.from_extended);
        const size_t index = LookupIndex(from);
        if (e.disabled) {
            table[index] = DisabledScanCode;
        } else {
            table[index] = MakeFullScanCode(
                static_cast<uint8_t>(e.to_scan_code), e.to_extended);
        }
    }
    return table;
}
