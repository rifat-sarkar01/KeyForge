#include "backend_hook/audio_output_switcher.h"

#include <windows.h>
#include <mmreg.h>
#include <mmdeviceapi.h>
#include <propsys.h>
#include <vector>

namespace {

// Windows exposes the policy client used by the Sound control panel, but does
// not publish a header for this small interface.  The vtable layout below is
// the established Windows 10/11 PolicyConfig contract; only
// SetDefaultEndpoint is used here.
struct IPolicyConfig : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE GetMixFormat(PCWSTR, WAVEFORMATEX**) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDeviceFormat(PCWSTR, INT, WAVEFORMATEX**) = 0;
    virtual HRESULT STDMETHODCALLTYPE ResetDeviceFormat(PCWSTR) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetDeviceFormat(PCWSTR, WAVEFORMATEX*, WAVEFORMATEX*) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetProcessingPeriod(PCWSTR, INT, PINT64, PINT64) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetProcessingPeriod(PCWSTR, PINT64) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetShareMode(PCWSTR, struct DeviceShareMode*) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetShareMode(PCWSTR, struct DeviceShareMode*) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetPropertyValue(PCWSTR, const PROPERTYKEY&, PROPVARIANT*) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetPropertyValue(PCWSTR, const PROPERTYKEY&, const PROPVARIANT*) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetDefaultEndpoint(PCWSTR, ERole) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetEndpointVisibility(PCWSTR, INT) = 0;
};

const CLSID kPolicyConfigClient =
    {0x870af99c, 0x171d, 0x4f9e, {0xaf, 0x0d, 0xe6, 0x3d, 0xf4, 0x0c, 0x2b, 0xc9}};
const IID kPolicyConfig =
    {0xf8679f50, 0x850a, 0x41cf, {0x9c, 0x72, 0x43, 0x0f, 0x29, 0x02, 0x90, 0xc8}};

class ComScope {
public:
    ComScope() : m_result(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED)) {}
    ~ComScope() { if (SUCCEEDED(m_result)) CoUninitialize(); }
private:
    HRESULT m_result;
};

}  // namespace

bool AudioOutputSwitcher::CycleDefaultRenderDevice() {
    ComScope com;

    IMMDeviceEnumerator* enumerator = nullptr;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
                                  CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                                  reinterpret_cast<void**>(&enumerator));
    if (FAILED(hr)) return false;

    IMMDeviceCollection* devices = nullptr;
    hr = enumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &devices);
    if (FAILED(hr)) { enumerator->Release(); return false; }

    IMMDevice* current = nullptr;
    hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &current);
    if (FAILED(hr)) { devices->Release(); enumerator->Release(); return false; }

    LPWSTR current_id = nullptr;
    current->GetId(&current_id);
    current->Release();

    UINT count = 0;
    devices->GetCount(&count);
    UINT next_index = count;
    for (UINT i = 0; i < count; ++i) {
        IMMDevice* device = nullptr;
        if (FAILED(devices->Item(i, &device))) continue;
        LPWSTR id = nullptr;
        if (SUCCEEDED(device->GetId(&id))) {
            const bool is_current = current_id && wcscmp(id, current_id) == 0;
            if (is_current && count > 1) {
                next_index = (i + 1) % count;
            }
            CoTaskMemFree(id);
        }
        device->Release();
        if (next_index != count) break;
    }
    CoTaskMemFree(current_id);
    LPWSTR next_id = nullptr;
    if (next_index != count) {
        IMMDevice* next = nullptr;
        if (SUCCEEDED(devices->Item(next_index, &next))) {
            next->GetId(&next_id);
            next->Release();
        }
    }
    devices->Release();
    enumerator->Release();
    if (!next_id) return false;

    IPolicyConfig* policy = nullptr;
    hr = CoCreateInstance(kPolicyConfigClient, nullptr, CLSCTX_ALL, kPolicyConfig,
                          reinterpret_cast<void**>(&policy));
    if (SUCCEEDED(hr)) {
        hr = policy->SetDefaultEndpoint(next_id, eConsole);
        if (SUCCEEDED(hr)) policy->SetDefaultEndpoint(next_id, eMultimedia);
        if (SUCCEEDED(hr)) policy->SetDefaultEndpoint(next_id, eCommunications);
        policy->Release();
    }
    CoTaskMemFree(next_id);
    return SUCCEEDED(hr);
}
