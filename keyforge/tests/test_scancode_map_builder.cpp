#include <cassert>
#include <cstdio>
#include <vector>
#include "engine/scancode_utils.h"
#include "backend_registry/scancode_map_writer.h"

void TestBuildBlobEmpty() {
    std::vector<RemapEntry> entries;
    auto blob = ScancodeMapWriter::BuildBlob(entries);
    // 8 bytes header + 4 bytes count + 0 real entries + 4 bytes terminator.
    assert(blob.size() == 16);
    assert(blob[0] == 0x00);
    assert(blob[8] == 0x01);
    assert(blob[9] == 0x00);
    assert(blob[10] == 0x00);
    assert(blob[11] == 0x00);
    assert(blob[12] == 0x00);
    assert(blob[13] == 0x00);
    assert(blob[14] == 0x00);
    assert(blob[15] == 0x00);
    printf("  PASS: BuildBlobEmpty\n");
}

void TestBuildBlobDisableEscape() {
    RemapEntry e;
    e.from_scan_code = 0x01;
    e.from_extended = false;
    e.to_scan_code = 0x00;
    e.to_extended = false;
    e.disabled = true;
    std::vector<RemapEntry> entries = {e};
    auto blob = ScancodeMapWriter::BuildBlob(entries);

    uint8_t expected[] = {
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x02, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x00
    };
    assert(blob.size() == sizeof(expected));
    for (size_t i = 0; i < sizeof(expected); ++i) {
        assert(blob[i] == expected[i]);
    }
    printf("  PASS: BuildBlobDisableEscape\n");
}

void TestBuildBlobSwapCapsCtrl() {
    RemapEntry e1;
    e1.from_scan_code = 0x3A;
    e1.from_extended = false;
    e1.to_scan_code = 0x1D;
    e1.to_extended = false;
    e1.disabled = false;

    RemapEntry e2;
    e2.from_scan_code = 0x01;
    e2.from_extended = false;
    e2.to_scan_code = 0x3A;
    e2.to_extended = false;
    e2.disabled = false;

    RemapEntry e3;
    e3.from_scan_code = 0x5C;
    e3.from_extended = true;
    e3.to_scan_code = 0x1D;
    e3.to_extended = true;
    e3.disabled = false;

    std::vector<RemapEntry> entries = {e1, e2, e3};
    auto blob = ScancodeMapWriter::BuildBlob(entries);

    assert(blob[8] == 0x04);
    assert(blob[9] == 0x00);
    assert(blob[10] == 0x00);
    assert(blob[11] == 0x00);

    assert(blob[12] == 0x1D);
    assert(blob[13] == 0x00);
    assert(blob[14] == 0x3A);
    assert(blob[15] == 0x00);

    assert(blob[16] == 0x3A);
    assert(blob[17] == 0x00);
    assert(blob[18] == 0x01);
    assert(blob[19] == 0x00);

    assert(blob[20] == 0x1D);
    assert(blob[21] == 0xE0);
    assert(blob[22] == 0x5C);
    assert(blob[23] == 0xE0);

    printf("  PASS: BuildBlobSwapCapsCtrl\n");
}

void TestBuildBlobDisabledIgnoresStaleToScanCode() {
    // Regression test: a disabled entry must produce a null (0x0000) target
    // scan code in the blob, even if to_scan_code/to_extended were left set
    // to some leftover value (e.g. from whatever was selected in the picker
    // before the "disable" checkbox was ticked). The live-hook lookup table
    // already got this right; BuildBlob did not.
    RemapEntry e;
    e.from_scan_code = 0x01;
    e.from_extended = false;
    e.to_scan_code = 0x1D;   // deliberately non-zero/stale
    e.to_extended = true;    // deliberately non-default/stale
    e.disabled = true;

    std::vector<RemapEntry> entries = {e};
    auto blob = ScancodeMapWriter::BuildBlob(entries);

    // bytes 12-15: new scan code (0x0000) then original scan code (0x0001)
    assert(blob[12] == 0x00);
    assert(blob[13] == 0x00);
    assert(blob[14] == 0x01);
    assert(blob[15] == 0x00);
    printf("  PASS: BuildBlobDisabledIgnoresStaleToScanCode\n");
}

void TestMakeFullScanCode() {
    assert(MakeFullScanCode(0x01, false) == 0x0001);
    assert(MakeFullScanCode(0x1D, true) == 0xE01D);
    assert(MakeFullScanCode(0x48, true) == 0xE048);
    assert(MakeFullScanCode(0x5B, true) == 0xE05B);
    printf("  PASS: TestMakeFullScanCode\n");
}

void TestGetBaseCode() {
    assert(GetBaseCode(0x0001) == 0x01);
    assert(GetBaseCode(0xE01D) == 0x1D);
    assert(GetBaseCode(0xE048) == 0x48);
    printf("  PASS: TestGetBaseCode\n");
}

void TestIsExtended() {
    assert(!IsExtended(0x0001));
    assert(IsExtended(0xE01D));
    assert(IsExtended(0xE048));
    assert(!IsExtended(0x003A));
    printf("  PASS: TestIsExtended\n");
}

int main() {
    printf("Running scancode_map_builder tests...\n");
    TestBuildBlobEmpty();
    TestBuildBlobDisableEscape();
    TestBuildBlobSwapCapsCtrl();
    TestBuildBlobDisabledIgnoresStaleToScanCode();
    TestMakeFullScanCode();
    TestGetBaseCode();
    TestIsExtended();
    printf("All scancode_map_builder tests passed!\n");
    return 0;
}
