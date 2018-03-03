// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http ://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Precompiled.h"
#include "Math/Math.h"
#include "Simd/Simd.h"
#include "Core/CVars.h"
#include "Sound/SoundSystem.h"

BE_NAMESPACE_BEGIN

static const int MaxSources = 32;

static CVar     s_khz(L"s_khz", L"44", CVar::Integer | CVar::Archive, L"");
static CVar     s_doppler(L"s_doppler", L"1.0", CVar::Float | CVar::Archive, L"");
static CVar     s_rolloff(L"s_rolloff", L"2.0", CVar::Float | CVar::Archive, L"");

bool SoundSystem::InitDevice(void *windowHandle) {
    HRESULT hr;

    BE_LOG(L"Initializing DirectSound...\n");

    this->hwnd = (HWND)windowHandle;

    BE_LOG(L"...creating DirectSound object: ");
    while ((hr = DirectSoundCreate8(nullptr, &dsDevice, nullptr)) != DS_OK) {
        if (hr == DSERR_ALLOCATED) {
            if (MessageBox(hwnd,
                L"The sound hardware is in use by another app.\n\n"
                L"Select Retry to try to start sound again or Cancel to run with no sound.",
                L"Sound system not available",
                MB_RETRYCANCEL | MB_SETFOREGROUND | MB_ICONEXCLAMATION) != IDRETRY) {
                BE_LOG(L"failed, hardware already in use\n");
                return false;
            }
        }

        BE_LOG(L"failed\n");
        return false;
    }
    BE_LOG(L"ok\n");

    DSCAPS dscaps;
    dscaps.dwSize = sizeof(dscaps);
    hr = dsDevice->GetCaps(&dscaps);
    if (hr == DS_OK) {
        // No HW driver
        if (dscaps.dwFlags & DSCAPS_EMULDRIVER) {
            BE_LOG(L"no DirectSound driver found\n");
            Shutdown();
            return false;
        }
    } else {
        BE_LOG(L"Couldn't get DirectSound caps\n");
    }

    BE_LOG(L"...setting DDSSCL_PRIORITY coop level: ");
    hr = dsDevice->SetCooperativeLevel(hwnd, DSSCL_PRIORITY);
    if (FAILED(hr)) {
        BE_LOG(L"failed\n");
        Shutdown();
        return false;
    }
    BE_LOG(L"ok\n");

    // Create primary sound
    DSBUFFERDESC dsbd;
    memset(&dsbd, 0, sizeof(dsbd));
    dsbd.dwSize = sizeof(dsbd);
    dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRL3D;// | DSBCAPS_CTRLVOLUME;
    //dsbd.dwBufferBytes = 0;
    //dsbd.lpwfxFormat = nullptr;
    //memset(&dsbd.guid3DAlgorithm, 0, sizeof(dsbd.guid3DAlgorithm));

    BE_LOG(L"...creating primary sound: ");
    if (dsDevice->CreateSoundBuffer(&dsbd, &dsPrimaryBuffer, nullptr) != DS_OK) {
        BE_LOG(L"failed\n");
        Shutdown();
        return false;
    }
    BE_LOG(L"ok\n");

    // Set the primary sound format
    const int khz = s_khz.GetInteger();

    WAVEFORMATEX fmt;
    fmt.cbSize          = 0;
    fmt.wFormatTag      = WAVE_FORMAT_PCM;
    fmt.nChannels       = 2;
    fmt.nSamplesPerSec  = (khz >= 44 ? 44100 : (khz >= 22 ? 22050 : 11025));
    fmt.wBitsPerSample  = 16;
    fmt.nBlockAlign     = fmt.nChannels * fmt.wBitsPerSample / 8;
    fmt.nAvgBytesPerSec = fmt.nSamplesPerSec * fmt.nBlockAlign;

    if (dsPrimaryBuffer->SetFormat(&fmt) != DS_OK) {
        BE_LOG(L"Couldn't set DirectSound primary sound format. Using default\n");
    }

    // Get actual format of the primary sound
    dsPrimaryBuffer->GetFormat(&fmt, sizeof(fmt), nullptr);

    //this->numChannels = fmt.nChannels;
    //this->sampleRates = fmt.nSamplesPerSec;
    //this->bitsWidth = fmt.wBitsPerSample;

    // Start mixer engine !
    dsPrimaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

    // Get the listener interface
    if (dsPrimaryBuffer->QueryInterface(IID_IDirectSound3DListener8, (void **)&dsListener3D) != DS_OK) {
        BE_LOG(L"failed to QueryInterface to get listener interface\n");
        Shutdown();
        return false;
    }

    // Set the listener properties
    DS3DLISTENER listenerProps;
    listenerProps.dwSize = sizeof(listenerProps);
    listenerProps.vPosition.x = 0.0f;
    listenerProps.vPosition.y = 0.0f;
    listenerProps.vPosition.z = 0.0f;
    listenerProps.vVelocity.x = 0.0f;
    listenerProps.vVelocity.y = 0.0f;
    listenerProps.vVelocity.z = 0.0f;
    listenerProps.vOrientFront.x = 0.0f;
    listenerProps.vOrientFront.y = 0.0f;
    listenerProps.vOrientFront.z = 1.0f;
    listenerProps.vOrientTop.x = 0.0f;
    listenerProps.vOrientTop.y = 1.0f;
    listenerProps.vOrientTop.z = 0.0f;
    listenerProps.flDistanceFactor = UnitToMeter(1.0f);
    // no attenuation needed because we attenuate volume manually
    listenerProps.flRolloffFactor = DS3D_MINROLLOFFFACTOR;//s_rolloff.GetFloat();
    listenerProps.flDopplerFactor = s_doppler.GetFloat();

    if (dsListener3D->SetAllParameters(&listenerProps, DS3D_IMMEDIATE) != DS_OK) {
        BE_LOG(L"failed to set listener all parameters\n");
        Shutdown();
        return false;
    }

    // Initialize sources
    for (int sourceIndex = 0; sourceIndex < MaxSources; sourceIndex++) {
        SoundSource *source = new SoundSource;
        sources.Append(source);
        freeSources.Append(source);
    }

    return true;
}

void SoundSystem::ShutdownDevice() {
    BE_LOG(L"Shutting down DirectSound...\n");

    for (int sourceIndex = 0; sourceIndex < sources.Count(); sourceIndex++) {
        SoundSource *source = sources[sourceIndex];
        delete source;
    }

    sources.Clear();
    freeSources.Clear();

    if (dsDevice) {
        BE_LOG(L"...setting DDSCL_NORMAL coop level\n");
        dsDevice->SetCooperativeLevel(hwnd, DSSCL_NORMAL);
    }

    if (dsListener3D) {
        dsListener3D->Release();
        dsListener3D = nullptr;
    }

    if (dsPrimaryBuffer) {
        BE_LOG(L"...stopping and releasing primary sound buffer\n");
        dsPrimaryBuffer->Stop();
        dsPrimaryBuffer->Release();
        dsPrimaryBuffer = nullptr;
    }

    if (dsDevice) {
        BE_LOG(L"...releasing DirectSound object\n");
        dsDevice->Release();
        dsDevice = nullptr;
    }
}

void SoundSystem::PlaceListenerInternal(const Vec3 &pos, const Vec3 &forward, const Vec3 &up) {
    dsListener3D->SetPosition(pos.x, pos.y, pos.z, DS3D_DEFERRED);
    dsListener3D->SetOrientation(-forward.x, -forward.y, -forward.z, up.x, up.y, up.z, DS3D_DEFERRED);
    dsListener3D->CommitDeferredSettings();
}

BE_NAMESPACE_END
