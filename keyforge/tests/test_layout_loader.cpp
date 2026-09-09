#include <cassert>
#include <cstdio>
#include <string>
#include "layout/layout_loader.h"

void TestLoadValidJson() {
    std::string json = R"({
        "layout_id": "test_layout",
        "display_name": "Test Layout",
        "key_count": 2,
        "unit_px": 50,
        "rows": 1,
        "keys": [
            {"id":"A","label":"A","scan_code":"0x1E","x":0.0,"y":0,"w":1.0,"h":1.0},
            {"id":"B","label":"B","scan_code":"0x1F","x":1.0,"y":0,"w":1.0,"h":1.0}
        ]
    })";

    Layout layout;
    bool ok = LayoutLoader::LoadFromJson(json, layout);
    assert(ok);
    assert(layout.layout_id == "test_layout");
    assert(layout.display_name == "Test Layout");
    assert(layout.key_count == 2);
    assert(layout.unit_px == 50);
    assert(layout.rows == 1);
    assert(layout.keys.size() == 2);
    assert(layout.keys[0].id == "A");
    assert(layout.keys[0].scan_code == 0x1E);
    assert(layout.keys[0].x == 0.0f);
    assert(layout.keys[1].id == "B");
    assert(layout.keys[1].scan_code == 0x1F);
    printf("  PASS: TestLoadValidJson\n");
}

void TestLoadExtendedKey() {
    std::string json = R"({
        "layout_id": "test",
        "display_name": "Test",
        "key_count": 1,
        "unit_px": 54,
        "rows": 1,
        "keys": [
            {"id":"DEL","label":"Del","scan_code":"0x53","extended":true,"x":0.0,"y":0,"w":1.0,"h":1.0}
        ]
    })";

    Layout layout;
    bool ok = LayoutLoader::LoadFromJson(json, layout);
    assert(ok);
    assert(layout.keys.size() == 1);
    assert(layout.keys[0].extended == true);
    assert(layout.keys[0].scan_code == 0x53);
    printf("  PASS: TestLoadExtendedKey\n");
}

void TestLoadHardwareOnlyKey() {
    std::string json = R"({
        "layout_id": "test",
        "display_name": "Test",
        "key_count": 1,
        "unit_px": 54,
        "rows": 1,
        "keys": [
            {"id":"FN","label":"Fn","scan_code":"HARDWARE_FN_NOT_OS_VISIBLE","x":0.0,"y":0,"w":1.0,"h":1.0}
        ]
    })";

    Layout layout;
    bool ok = LayoutLoader::LoadFromJson(json, layout);
    assert(ok);
    assert(layout.keys.size() == 1);
    assert(layout.keys[0].hardware_only == true);
    printf("  PASS: TestLoadHardwareOnlyKey\n");
}

void TestLoadEmptyKeys() {
    std::string json = R"({
        "layout_id": "empty",
        "display_name": "Empty",
        "key_count": 0,
        "unit_px": 54,
        "rows": 0,
        "keys": []
    })";

    Layout layout;
    bool ok = LayoutLoader::LoadFromJson(json, layout);
    assert(!ok);
    printf("  PASS: TestLoadEmptyKeys\n");
}

void TestLoadMissingField() {
    std::string json = R"({"layout_id":"broken"})";
    Layout layout;
    bool ok = LayoutLoader::LoadFromJson(json, layout);
    assert(!ok);
    printf("  PASS: TestLoadMissingField\n");
}

int main() {
    printf("Running layout_loader tests...\n");
    TestLoadValidJson();
    TestLoadExtendedKey();
    TestLoadHardwareOnlyKey();
    TestLoadEmptyKeys();
    TestLoadMissingField();
    printf("All layout_loader tests passed!\n");
    return 0;
}
