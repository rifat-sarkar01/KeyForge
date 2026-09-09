#pragma once

// Cycles through Windows' currently active playback devices and makes the
// next one the default device for console, multimedia, and communications.
class AudioOutputSwitcher {
public:
    static bool CycleDefaultRenderDevice();
};
