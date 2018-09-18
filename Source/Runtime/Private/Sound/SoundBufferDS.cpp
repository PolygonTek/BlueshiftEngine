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
#include "Simd/Simd.h"
#include "Sound/Pcm.h"
#include "Sound/SoundSystem.h"

BE_NAMESPACE_BEGIN

SoundBuffer::SoundBuffer() {
    dsBuffer = nullptr;
    ds3dBuffer = nullptr;
}

SoundBuffer::~SoundBuffer() {
    Purge();
}

void SoundBuffer::Purge() {
    if (ds3dBuffer) {
        ds3dBuffer->Release();
        ds3dBuffer = nullptr;
    }

    if (dsBuffer) {
        dsBuffer->Release();
        dsBuffer = nullptr;
    }
}

bool SoundBuffer::CreateStaticBuffer(int numChannels, int bitsWidth, int sampleRates, int size, const byte *data) {
    Purge();

    DWORD dwFlags = DSBCAPS_STATIC | DSBCAPS_CTRLVOLUME | DSBCAPS_GETCURRENTPOSITION2;
    if (numChannels == 1) {
        dwFlags |= DSBCAPS_CTRL3D;
    }

    WAVEFORMATEX fmt;
    fmt.cbSize          = 0;
    fmt.wFormatTag      = WAVE_FORMAT_PCM;
    fmt.nChannels       = numChannels;
    fmt.nSamplesPerSec  = sampleRates;
    fmt.wBitsPerSample  = bitsWidth;
    fmt.nBlockAlign     = fmt.nChannels * fmt.wBitsPerSample / 8;
    fmt.nAvgBytesPerSec = fmt.nSamplesPerSec * fmt.nBlockAlign;

    DSBUFFERDESC dsbd;
    memset(&dsbd.guid3DAlgorithm, 0, sizeof(dsbd.guid3DAlgorithm));
    dsbd.dwSize         = sizeof(dsbd);
    dsbd.dwFlags        = dwFlags;
    dsbd.dwReserved     = 0;
    dsbd.dwBufferBytes  = size;
    dsbd.lpwfxFormat    = &fmt;

    HRESULT hr = soundSystem.dsDevice->CreateSoundBuffer(&dsbd, &dsBuffer, nullptr);
    if (hr != DS_OK) {
        BE_WARNLOG(L"Couldn't create DirectSound buffer\n");
        return false;
    }

    DWORD lockedSize;
    void *lockedPtr;

    if (SUCCEEDED(dsBuffer->Lock(0, size, &lockedPtr, &lockedSize, nullptr, nullptr, 0))) {
        simdProcessor->Memcpy(lockedPtr, data, lockedSize);
        dsBuffer->Unlock(lockedPtr, lockedSize, nullptr, 0);
    } else {
        dsBuffer->Release();
        return false;
    }

    if (numChannels == 1) {
        dsBuffer->QueryInterface(IID_IDirectSound3DBuffer, (void **)&ds3dBuffer);
    }

    return true;
}

bool SoundBuffer::CreateStreamBuffer(int numChannels, int bitsWidth, int sampleRates) {
    Purge();

    streamBufferSize = numChannels * sampleRates * (bitsWidth >> 3) * StreamBufferSeconds;
    streamBufferCount = MaxStreamBuffers;

    DWORD dwFlags = DSBCAPS_CTRLVOLUME | DSBCAPS_GETCURRENTPOSITION2;
    if (numChannels == 1) {
        dwFlags |= DSBCAPS_CTRL3D;
    }

    WAVEFORMATEX fmt;
    fmt.cbSize          = 0;
    fmt.wFormatTag      = WAVE_FORMAT_PCM;
    fmt.nChannels       = numChannels;
    fmt.nSamplesPerSec  = sampleRates;
    fmt.wBitsPerSample  = bitsWidth;
    fmt.nBlockAlign     = fmt.nChannels * fmt.wBitsPerSample / 8;
    fmt.nAvgBytesPerSec = fmt.nSamplesPerSec * fmt.nBlockAlign;

    DSBUFFERDESC dsbd;
    memset(&dsbd.guid3DAlgorithm, 0, sizeof(dsbd.guid3DAlgorithm));
    dsbd.dwSize         = sizeof(dsbd);
    dsbd.dwFlags        = dwFlags;
    dsbd.dwReserved     = 0;
    dsbd.dwBufferBytes  = streamBufferSize * streamBufferCount;
    dsbd.lpwfxFormat    = &fmt;

    HRESULT hr = soundSystem.dsDevice->CreateSoundBuffer(&dsbd, &dsBuffer, nullptr);
    if (hr != DS_OK) {
        BE_WARNLOG(L"Couldn't create DirectSound buffer\n");
        return false;
    }

    if (numChannels == 1) {
        dsBuffer->QueryInterface(IID_IDirectSound3DBuffer, (void **)&ds3dBuffer);
    }

    return true;
}

bool SoundBuffer::DuplicateBuffer(SoundBuffer *originalSoundBuffer) {
    HRESULT hr = soundSystem.dsDevice->DuplicateSoundBuffer(originalSoundBuffer->dsBuffer, &dsBuffer);
    if (hr != DS_OK) {
        BE_WARNLOG(L"Couldn't duplicate DirectSound buffer\n");
        return false;
    }

    if (originalSoundBuffer->ds3dBuffer) {
        dsBuffer->QueryInterface(IID_IDirectSound3DBuffer, (void **)&ds3dBuffer);
    }

    return true;
}

BE_NAMESPACE_END