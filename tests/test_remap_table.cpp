#include <algorithm>
#include <cassert>
#include <cstdio>
#include <string>
#include "engine/remap_table.h"
#include "layout/layout_loader.h"

Layout MakeTestLayout() {
    std::string json = R"({
        "layout_id": "test",
        "display_name": "Test",
        "key_count": 3,
        "unit_px": 54,
        "rows": 1,
        "keys": [
            {"id":"A","label":"A","scan_code":"0x1E","x":0.0,"y":0,"w":1.0,"h":1.0},
            {"id":"B","label":"B","scan_code":"0x1F","x":1.0,"y":0,"w":1.0,"h":1.0},
            {"id":"C","label":"C","scan_code":"0x20","x":2.0,"y":0,"w":1.0,"h":1.0}
        ]
    })";
    Layout layout;
    LayoutLoader::LoadFromJson(json, layout);
    return layout;
}

void TestAddAndRemove() {
    RemapTable table;
    auto layout = MakeTestLayout();

    assert(!table.HasChanges());

    table.AddRemap("A", "A", 0x1F, false, false);
    assert(table.HasChanges());
    assert(table.GetPending().size() == 1);
    assert(table.GetPending()[0].key_id == "A");

    table.RemoveRemap("A");
    assert(!table.HasChanges());
    printf("  PASS: TestAddAndRemove\n");
}

void TestOverwriteRemap() {
    RemapTable table;
    auto layout = MakeTestLayout();

    // Remap A to B first, then change our mind and remap A to C instead.
    // The label must update too, not just the scan code - a user re-picking
    // a target should see the new target's name, not the first one they
    // picked.
    table.AddRemap("A", "B", 0x1F, false, false);
    table.AddRemap("A", "C", 0x20, false, true);
    assert(table.GetPending().size() == 1);
    assert(table.GetPending()[0].to_scan_code == 0x20);
    assert(table.GetPending()[0].disabled == true);
    assert(table.GetPending()[0].to_label == "C");
    printf("  PASS: TestOverwriteRemap\n");
}

void TestClear() {
    RemapTable table;
    auto layout = MakeTestLayout();

    table.AddRemap("A", "A", 0x1F, false, false);
    table.AddRemap("B", "B", 0x20, false, false);
    assert(table.HasChanges());

    table.Clear();
    assert(!table.HasChanges());
    assert(table.GetPending().empty());
    printf("  PASS: TestClear\n");
}

void TestBuildEntries() {
    RemapTable table;
    auto layout = MakeTestLayout();

    table.AddRemap("A", "A", 0x1F, false, false);
    auto entries = table.BuildEntries(layout);
    assert(entries.size() == 1);
    assert(entries[0].from_scan_code == 0x1E);
    assert(entries[0].to_scan_code == 0x1F);
    assert(entries[0].from_extended == false);
    printf("  PASS: TestBuildEntries\n");
}

void TestBuildEntriesDisabled() {
    RemapTable table;
    auto layout = MakeTestLayout();

    table.AddRemap("A", "A", 0, false, true);
    auto entries = table.BuildEntries(layout);
    assert(entries.size() == 1);
    assert(entries[0].disabled == true);
    printf("  PASS: TestBuildEntriesDisabled\n");
}

void TestGetLookupTable() {
    RemapTable table;
    auto layout = MakeTestLayout();

    table.AddRemap("A", "A", 0x1F, false, false);
    auto lut = table.GetLookupTable(layout);
    assert(lut[0x1E] == 0x1F);
    assert(lut[0x1F] == 0);
    printf("  PASS: TestGetLookupTable\n");
}

void TestGetLookupTableExtendedKey() {
    RemapTable table;
    std::string json = R"({
        "layout_id":"test", "display_name":"Test", "key_count":1,
        "unit_px":54, "rows":1,
        "keys":[{"id":"DEL","label":"Del","scan_code":"0x53","extended":true,"x":0,"y":0,"w":1,"h":1}]
    })";
    Layout layout;
    assert(LayoutLoader::LoadFromJson(json, layout));

    table.AddRemap("DEL", "Home", 0x47, true, false);
    const auto lut = table.GetLookupTable(layout);
    assert(lut[LookupIndex(0xE053)] == 0xE047);
    assert(lut[LookupIndex(0x0053)] == 0);
    printf("  PASS: TestGetLookupTableExtendedKey\n");
}

void TestPauseBreakHookNormalization() {
    // A low-level hook must classify Pause by VK_PAUSE, not by its 0x45 byte.
    // Ctrl+Pause is Break (VK_CANCEL / control-break) and Num Lock remains a
    // normal 0x45 scan-code source.
    assert(ClassifyHookSource(PauseVirtualKey, false) == HookSourceKind::Pause);
    assert(ClassifyHookSource(PauseVirtualKey, true) == HookSourceKind::Break);
    assert(ClassifyHookSource(CancelVirtualKey, false) == HookSourceKind::Break);
    assert(ClassifyHookSource(NumLockVirtualKey, false) == HookSourceKind::ScanCode);
    assert(SourceScanCodeForHook(0x45, false, HookSourceKind::Pause) == 0xE0E1);
    assert(SourceScanCodeForHook(0x45, false, HookSourceKind::ScanCode) == 0x0045);
    printf("  PASS: TestPauseBreakHookNormalization\n");
}

void TestNonexistentKeyIgnored() {
    RemapTable table;
    auto layout = MakeTestLayout();

    table.AddRemap("NONEXISTENT", "X", 0x1E, false, false);
    auto entries = table.BuildEntries(layout);
    assert(entries.empty());
    printf("  PASS: TestNonexistentKeyIgnored\n");
}

void TestJsonRoundTrip() {
    RemapTable table;
    auto layout = MakeTestLayout();

    table.AddRemap("A", "A", 0x1F, true, false);
    table.AddRemap("B", "B", 0x20, false, true);

    std::string json = table.ToJson();

    RemapTable reloaded;
    reloaded.FromJson(json, layout);

    const auto& pending = reloaded.GetPending();
    assert(pending.size() == 2);

    auto it_a = std::find_if(pending.begin(), pending.end(),
        [](const PendingChange& p) { return p.key_id == "A"; });
    assert(it_a != pending.end());
    assert(it_a->to_scan_code == 0x1F);
    assert(it_a->to_extended == true);
    assert(it_a->disabled == false);

    auto it_b = std::find_if(pending.begin(), pending.end(),
        [](const PendingChange& p) { return p.key_id == "B"; });
    assert(it_b != pending.end());
    assert(it_b->to_scan_code == 0x20);
    assert(it_b->to_extended == false);
    assert(it_b->disabled == true);

    printf("  PASS: TestJsonRoundTrip\n");
}

int main() {
    printf("Running remap_table tests...\n");
    TestAddAndRemove();
    TestOverwriteRemap();
    TestClear();
    TestBuildEntries();
    TestBuildEntriesDisabled();
    TestGetLookupTable();
    TestGetLookupTableExtendedKey();
    TestPauseBreakHookNormalization();
    TestNonexistentKeyIgnored();
    TestJsonRoundTrip();
    printf("All remap_table tests passed!\n");
    return 0;
}
