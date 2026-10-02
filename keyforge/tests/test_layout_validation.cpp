#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_set>

#include <windows.h>

#include "engine/function_keys.h"
#include "engine/scancode_utils.h"
#include "layout/layout_loader.h"

#ifndef KEYFORGE_LAYOUTS_DIR
#error "KEYFORGE_LAYOUTS_DIR must point at the shipped data/layouts directory"
#endif

namespace fs = std::filesystem;

// Release builds define NDEBUG, which turns assert() into a no-op. Failures
// are counted here instead so ctest reports a non-zero exit code in every
// configuration.
namespace {
int g_failures = 0;
}  // namespace

// A wrong `extended` flag or scan-code byte in a shipped layout still loads
// perfectly well - it just lands on another key's slot in the 512-entry
// lookup table. The key whose slot was taken can then never be remapped, and
// it silently picks up the other key's remap instead.
void TestNoLookupIndexCollisions() {
    const int failures_before = g_failures;
    int layouts_checked = 0;

    for (const fs::directory_entry& entry :
         fs::directory_iterator(KEYFORGE_LAYOUTS_DIR)) {
        if (entry.path().extension() != ".json") continue;

        std::ifstream in(entry.path());
        std::stringstream buffer;
        buffer << in.rdbuf();

        Layout layout;
        if (!LayoutLoader::LoadFromJson(buffer.str(), layout)) {
            printf("  FAIL: %s did not load\n",
                   entry.path().filename().string().c_str());
            ++g_failures;
            continue;
        }
        ++layouts_checked;

        std::unordered_set<size_t> used_slots;
        for (const KeyCell& key : layout.keys) {
            if (key.hardware_only) continue;
            const uint16_t full = MakeFullScanCode(
                static_cast<uint8_t>(key.scan_code), key.extended);
            if (!used_slots.insert(LookupIndex(full)).second) {
                printf("  FAIL: %s: '%s' (0x%04X) shares a lookup slot with "
                       "another key\n",
                       entry.path().filename().string().c_str(),
                       key.id.c_str(), full);
                ++g_failures;
            }
        }
    }

    if (layouts_checked == 0) {
        printf("  FAIL: no .json layouts found in %s\n", KEYFORGE_LAYOUTS_DIR);
        ++g_failures;
    } else if (g_failures == failures_before) {
        printf("  PASS: TestNoLookupIndexCollisions (%d layouts)\n",
               layouts_checked);
    }
}

// A function key only works as a remap target if Windows can translate its
// scan code back into a virtual key: the hook swallows the user's real key
// and injects the target as a scan code. A target Windows cannot translate
// (Power, Wake) leaves the user holding a dead key.
void TestFunctionKeysAreInjectable() {
    const int failures_before = g_failures;

    for (const FunctionKeyDef& key : GetFunctionKeys()) {
        if (key.scan_code == AudioOutputSwitchAction) continue;
        const uint16_t full = MakeFullScanCode(
            static_cast<uint8_t>(key.scan_code), key.extended);
        if (MapVirtualKey(full, MAPVK_VSC_TO_VK_EX) == 0) {
            printf("  FAIL: %s (0x%04X) has no virtual-key translation\n",
                   key.id, full);
            ++g_failures;
        }
    }

    if (g_failures == failures_before) {
        printf("  PASS: TestFunctionKeysAreInjectable\n");
    }
}

int main() {
    printf("Running layout validation tests...\n");
    TestNoLookupIndexCollisions();
    TestFunctionKeysAreInjectable();
    if (g_failures != 0) {
        printf("%d layout validation check(s) failed!\n", g_failures);
        return 1;
    }
    printf("All layout validation tests passed!\n");
    return 0;
}
