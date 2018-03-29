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
#include "Platform/PlatformTime.h"
#include "Simd/Simd.h"
#include "Sound/SoundSystem.h"

BE_NAMESPACE_BEGIN

SoundSource::SoundSource() {
    sound = nullptr;
    dsBuffer = nullptr;
    ds3dBuffer = nullptr;
}

SoundSource::~SoundSource() {
    if (pcm.IsOpened()) {
        pcm.Close();
    }
}

void SoundSource::Init(Sound *sound) {
    this->sound = sound;
    this->dsBuffer = sound->soundBuffer->dsBuffer;
    this->ds3dBuffer = sound->soundBuffer->ds3dBuffer;

    if (!sound->isStream) {
        if (sound->playingTime > 0) {
            dsBuffer->SetCurrentPosition(sound->ByteOffset());
        }
    } else {
        streamEnded = false;
    }

    if (sound->NumChannels() == 1 && !sound->localSound) {
        ds3dBuffer->SetMinDistance(DS3D_DEFAULTMINDISTANCE, DS3D_DEFERRED);
        ds3dBuffer->SetMaxDistance(DS3D_DEFAULTMAXDISTANCE, DS3D_DEFERRED);
    }

    Update();
}

void SoundSource::Update() {
    if (!sound) {
        return;
    }

    if (sound->isStream) {
        UpdateStream();
    }

    if (sound->NumChannels() == 1 && !sound->localSound) {
        ds3dBuffer->SetPosition(sound->origin.x, sound->origin.y, sound->origin.z, DS3D_DEFERRED);
        //ds3dBuffer->SetVelocity(sound->velocity.x, sound->velocity.y, sound->velocity.z, DS3D_DEFERRED);

        // Do linear attenuation
        float distance = soundSystem.listenerPosition.Distance(sound->origin);
        Clamp(distance, sound->minDistance, sound->maxDistance);
        float gain = (1.0f - (distance - sound->minDistance) / (sound->maxDistance - sound->minDistance));
        SetVolume(sound->volume * gain * SoundSystem::s_volume.GetFloat());
    } else {
        SetVolume(sound->volume * SoundSystem::s_volume.GetFloat());
    }
}

void SoundSource::UpdateStream() {
    if (!pcm.IsOpened()) {
        pcm.Open(sound->hashName);
    }

    int pcmBufferSize = sound->soundBuffer->streamBufferSize;
    byte *pcmBuffer = (byte *)_alloca16(pcmBufferSize);

    if (IsFinished()) {
        pcm.Seek(sound->ByteOffset());

        streamWriteOffset = 0;

        for (int bufferIndex = 0; bufferIndex < sound->soundBuffer->streamBufferCount; bufferIndex++) {
            int readSize = pcm.Read(pcmBufferSize, pcmBuffer);
            if (sound->looping) {
                while (readSize < pcmBufferSize) {
                    pcm.Seek(0);
                    readSize += pcm.Read(pcmBufferSize - readSize, pcmBuffer + readSize);
                }
            } else {
                if (readSize < pcmBufferSize) {
                    memset(pcmBuffer + readSize, 0, pcmBufferSize - readSize);

                    if (!streamEnded) {
                        streamEnded = true;
                    }
                }
            }

            DWORD lockedSize;
            void *lockedPtr;
            HRESULT hr = dsBuffer->Lock(bufferIndex * pcmBufferSize, pcmBufferSize, &lockedPtr, &lockedSize, nullptr, nullptr, 0);
            if (!SUCCEEDED(hr)) {
                BE_ERRLOG(L"SoundSource::UpdateStream: failed to lock buffer\n");
                return;
            }

            simdProcessor->Memcpy(lockedPtr, pcmBuffer, lockedSize);
            dsBuffer->Unlock(lockedPtr, lockedSize, nullptr, 0);
        }
    } else {
        DWORD currentPos;
        dsBuffer->GetCurrentPosition(&currentPos, nullptr);

        int delta = currentPos - streamWriteOffset;
        if (delta < 0) {
            delta += sound->soundBuffer->streamBufferSize * sound->soundBuffer->streamBufferCount;
        }
        
        int numProcessedBuffers = delta / sound->soundBuffer->streamBufferSize;
        while (numProcessedBuffers > 0) {
            int readSize = pcm.Read(pcmBufferSize, pcmBuffer);
            if (sound->looping) {
                while (readSize < pcmBufferSize) {
                    pcm.Seek(0);
                    readSize += pcm.Read(pcmBufferSize - readSize, pcmBuffer + readSize);
                }
            } else {
                if (readSize < pcmBufferSize) {
                    memset(pcmBuffer + readSize, 0, pcmBufferSize - readSize);

                    if (!streamEnded) {
                        dsBuffer->Stop();
                        dsBuffer->Play(0, 0, 0);
                        streamEnded = true;
                    }
                }
            }
        
            DWORD lockedSize;
            void *lockedPtr;
            HRESULT hr = dsBuffer->Lock(streamWriteOffset, sound->soundBuffer->streamBufferSize, &lockedPtr, &lockedSize, nullptr, nullptr, 0);
            if (!SUCCEEDED(hr)) {
                BE_ERRLOG(L"SoundSource::UpdateStream: failed to lock buffer\n");
                return;
            }            

            simdProcessor->Memcpy(lockedPtr, pcmBuffer, lockedSize);
            dsBuffer->Unlock(lockedPtr, lockedSize, nullptr, 0);

            // add the write offset as block size
            streamWriteOffset += sound->soundBuffer->streamBufferSize;
            // wrap the write offset because it's circular buffer
            streamWriteOffset %= sound->soundBuffer->streamBufferSize * sound->soundBuffer->streamBufferCount;

            numProcessedBuffers--;
        }
    }
}

bool SoundSource::IsFinished() {
    if (!sound) {
        return true;
    }

    DWORD status;
    if (SUCCEEDED(dsBuffer->GetStatus(&status))) {
        if ((status & DSBSTATUS_PLAYING) == DSBSTATUS_PLAYING) {
            return false;
        }
    }
    return true;
}

bool SoundSource::Stop() {
    if (!sound) {
        return false;
    }
    dsBuffer->Stop();
    dsBuffer->SetCurrentPosition(0);

    if (pcm.IsOpened()) {
        pcm.Close();
    }

    return true;
}

void SoundSource::Pause() {
    if (!sound) {
        return;
    }
    dsBuffer->Stop();
}

void SoundSource::Play() {
    if (!sound) {
        return;
    }

    if (ds3dBuffer) {
        if (sound->localSound) {
            ds3dBuffer->SetMode(DS3DMODE_DISABLE, DS3D_DEFERRED);
        } else {
            ds3dBuffer->SetMode(DS3DMODE_NORMAL, DS3D_DEFERRED);
        }
    }

    dsBuffer->Play(0, 0, ((sound->isStream && !streamEnded) || sound->looping) ? DSBPLAY_LOOPING : 0);
}

uint32_t SoundSource::GetCurrentOffset() const {
    if (!sound) {
        return 0;
    }
    DWORD position;
    dsBuffer->GetCurrentPosition(&position, nullptr);

    return position;
}

void SoundSource::SetCurrentOffset(uint32_t position) {
    if (!sound) {
        return;
    }

    if (sound->isStream) {
        if (!IsFinished()) {
            Stop();
            Update();
            Play();
        }
    } else {
        dsBuffer->SetCurrentPosition(position);
    }
}

void SoundSource::SetVolume(float volume) {
    if (!sound) {
        return;
    }
    LONG logVolume;

    if (volume <= 0) {
        logVolume = DSBVOLUME_MIN;
    } else if (volume >= 1.0f) {
        logVolume = 0;
    } else {
        logVolume = Clamp<LONG>((LONG)(2000.0f * Math::Log(10, volume)), DSBVOLUME_MIN, 0);
    }

    dsBuffer->SetVolume(logVolume);
}

BE_NAMESPACE_END