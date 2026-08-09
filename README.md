# KeyForge

KeyForge is a native Windows keyboard remapper. It displays a virtual keyboard,
lets you assign a different key or media action to each supported key, and can
apply remaps either while the app is running or through the Windows registry.

## Features

- Live keyboard remapping using a `WH_KEYBOARD_LL` hook.
- Media-key targets, including volume, playback, browser, and application keys.
- Built-in keyboard layouts: 40%, 60%, 65%, 75%, TKL, 1800 Compact / 96%, and
  full-size ANSI.
- Layout navigation from the app header using the `<` and `>` buttons.
- Per-user profiles saved in `%APPDATA%\KeyForge\profiles`.
- System tray controls for opening, pausing, and quitting the app.
- Optional permanent remaps through Windows' `Scancode Map` registry value.
- Self-contained Release executable: built-in layout JSON files are embedded as
  Windows resources.

## Requirements

- Windows 10 or later
- CMake 3.20+
- A C++20-capable MSVC toolchain (Visual Studio 2022 or later)

KeyForge uses Windows system components such as Direct2D and DirectWrite; no
third-party runtime DLLs are required for the Release executable.

## Build

Run these commands from a Visual Studio x64 Developer Command Prompt:

```powershell
cmake -S . -B out\release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build out\release --parallel
ctest --test-dir out\release --output-on-failure
```

The executable is created at:

```text
out\release\KeyForge.exe
```

## Usage

1. Run `KeyForge.exe`.
2. Select a physical key in the displayed layout.
3. Choose the target key or media action.
4. Click **Save && Apply** to enable the remap immediately and save it to the
   active profile.
5. Click **Make Permanent** only when you want Windows to apply the remap after
   reboot. Administrator approval and a Windows restart are required.

Use the tray icon to pause the live hook, reopen the window, or quit KeyForge.
Closing the main window hides it to the tray.

## Pause/Break notes

Pause/Break is unusual: it uses a multi-byte legacy `E1` scan-code sequence
instead of a normal make/break pair. KeyForge identifies live Pause input by
its virtual-key identity and keeps it distinct from Num Lock, which also uses
scan code `0x45`.

Pause/Break remaps must be used with **Save && Apply** while KeyForge is
running. KeyForge intentionally prevents storing a Pause/Break remap as a
permanent `Scancode Map`, because the Windows registry mapping format cannot
represent its special sequence safely.

For hardware diagnostics, KeyForge writes relevant Raw Input events to:

```text
%APPDATA%\KeyForge\raw-input.log
```

This monitor is passive and does not remap or suppress input.

## Project structure

```text
src/
  backend_hook/       Live keyboard hook and Raw Input diagnostics
  backend_registry/   Permanent Windows registry remaps
  engine/             Scan-code and remap-table logic
  layout/             Layout parsing and types
  profiles/           Profile persistence
  tray/               Notification-area icon
  ui/                 Direct2D user interface
data/layouts/         Source JSON for embedded keyboard layouts
tests/                Unit tests
third_party/          Bundled dependencies
```

## Safety

Use **Restore Defaults** to remove all remaps from the current profile and
clear the permanent registry map. If a permanent map was active, restart
Windows afterward.
