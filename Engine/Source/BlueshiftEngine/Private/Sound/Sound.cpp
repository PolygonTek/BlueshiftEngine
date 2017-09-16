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
#include "Core/Heap.h"
#include "Core/CVars.h"
#include "Core/Cmds.h"
#include "Platform/PlatformTime.h"
#include "Sound/SoundSystem.h"

BE_NAMESPACE_BEGIN

#define MAX_REUSABLE_SOUND 5

Sound::Sound() {
    refCount = 0;
    permanence = false;
    originalSound = nullptr;
    soundBuffer = nullptr;
    soundSource = nullptr;
    playingTime = 0;
}

Sound::~Sound() {
    Purge();
}

int Sound::ByteOffset() const { 
    float fraction = ((float)playingTime / duration);
    
    int byteOffset = AlignDown((int)(bytes * fraction), numChannels * (bitsWidth >> 3));    
    Clamp(byteOffset, 0, bytes);

    return byteOffset;
}

void Sound::Purge() {
    // If this sound have a original sound pointer, this is the duplicated one
    if (originalSound) {
        if (dupNode.InList()) {
            dupNode.Remove();
        }
    }

    SAFE_DELETE(soundBuffer);
}

void Sound::Create(Pcm &pcm) {
    Purge();

    numChannels     = pcm.NumChannels();
    sampleRates     = pcm.SampleRates();
    bitsWidth       = pcm.BitsWidth();
    bytes           = pcm.DataSize();
    duration        = SEC2MS(pcm.Duration());
    playingTime     = 0;

    if (pcm.Duration() <= 6.0f) {
        CreateStatic(pcm);
    } else {
        CreateStream(pcm);
    }
}

void Sound::CreateStatic(Pcm &pcm) {
    isStream = false;

    soundBuffer = new SoundBuffer;

    int pcmDataSize = 0;
    byte *pcmData = nullptr;
    
    if (pcm.IsLoaded()) {
        pcmDataSize = pcm.DataSize();
        pcmData = pcm.GetData();
    } else if (pcm.IsOpened()) {
        pcmDataSize = pcm.DataSize();
        pcmData = (byte *)Mem_Alloc16(pcmDataSize);
        pcm.Read(pcmDataSize, pcmData);
    } else {
        BE_WARNLOG(L"Empty PCM data '%hs'\n", name.c_str());
        return;
    }

    soundBuffer->CreateStaticBuffer(numChannels, bitsWidth, sampleRates, pcmDataSize, pcmData);

    if (pcm.IsOpened()) {
        Mem_AlignedFree(pcmData);
    }
}

void Sound::CreateStream(Pcm &pcm) {
    isStream = true;

    soundBuffer = nullptr;
}

Sound *Sound::Instantiate() {
    Sound *lruSound = nullptr;
    int reusableCount = 0;

    // Find a reusable duplicated sound buffer which is currently not playing
    for (LinkList<Sound> *node = dupNode.NextNode(); node; node = node->NextNode()) {
        Sound *s = node->Owner();
        if (!s->playNode.InList()) {
            return s;
        }

        // Only non-looping sound can be reusable
        if (!s->looping) {
            if (!lruSound) {
                lruSound = s;
            } else {
                if (s->playingTime > lruSound->playingTime) {
                    lruSound = s;
                }
            }

            reusableCount++;
        }
    }

    // Forces to stop and reuse duplicated sound if maximum reusable count is reached
    if (reusableCount >= MAX_REUSABLE_SOUND) {
        lruSound->Stop();
        return lruSound;
    }

    // If not found, duplicate original sound buffer
    Sound *s = new Sound;
    s->refCount = 1;
    s->DuplicateFrom(this);
    s->dupNode.SetOwner(s);
    s->dupNode.AddToEnd(dupNode);

    return s;
}

void Sound::DuplicateFrom(Sound *sound) {
    Purge();

    originalSound   = sound;
    hashName        = sound->hashName;
    name            = sound->name;
    numChannels     = sound->numChannels;
    sampleRates     = sound->sampleRates;
    bitsWidth       = sound->bitsWidth;
    duration        = sound->duration;
    bytes           = sound->bytes;
    isStream        = sound->isStream;

    if (isStream) {
        soundBuffer = new SoundBuffer;
        soundBuffer->CreateStreamBuffer(numChannels, bitsWidth, sampleRates);
    } else {
        soundBuffer = new SoundBuffer;
        soundBuffer->DuplicateBuffer(sound->soundBuffer);
    }
}

void Sound::Play2D(float volume, bool looping) {
    this->localSound    = true;
    this->playingTime   = 0;
    this->volume        = volume;
    this->looping       = looping;

    this->playNode.SetOwner(this);
    this->playNode.AddToEnd(soundSystem.soundPlayLinkList);
}

void Sound::Play3D(const Vec3 &origin, float minDistance, float maxDistance, float volume, bool looping) {
    this->localSound    = false;
    this->playingTime   = 0;
    this->volume        = volume;
    this->looping       = looping;

    this->origin        = origin;
    this->minDistance   = minDistance;
    this->maxDistance   = maxDistance;
    
    this->playNode.SetOwner(this);
    this->playNode.AddToEnd(soundSystem.soundPlayLinkList);
}

void Sound::Stop() {
    if (playNode.InList()) {
        playNode.Remove();

        if (soundSource) {
            soundSource->Stop();
            soundSource = nullptr;
        }
    }

    playingTime = 0;
}

bool Sound::IsPlaying() const {
    return playNode.InList();
}

void Sound::SetVolume(float vol) {
    volume = vol;

    if (soundSource) {
        soundSource->SetVolume(volume * SoundSystem::s_volume.GetFloat());
    }
}

uint32_t Sound::GetPlayingTime() const {
    if (soundSource) {
        if (!isStream) {
            float fraction = (float)soundSource->GetCurrentOffset() / bytes;
            return (uint32_t)(fraction * duration);
        }
    }

    return playingTime;
}

void Sound::SetPlayingTime(uint32_t time) {
    if (time > duration) {
        return;
    }

    playingTime = time;

    if (soundSource) {
        int32_t offset = bytes * ((float)time / duration);

        soundSource->SetCurrentOffset(offset);
    }
}

void Sound::UpdatePosition(const Vec3 &origin) {
    this->origin = origin;
}

bool Sound::Load(const char *filename) {
    Purge();

    BE_LOG(L"Loading sound '%hs'...\n", filename);

    Pcm pcm;
    if (!pcm.Open(filename)) {
        return false;
    }

    Create(pcm);
    
    return true;
}

bool Sound::Reload() {
    Sound *sound = this;
    if (originalSound) {
        sound = originalSound;
    }

    Str _hashName = hashName;
    bool ret = Load(_hashName);

    for (LinkList<Sound> *node = sound->dupNode.NextNode(); node; node = node->NextNode()) {
        Sound *s = node->Owner();
        s->DuplicateFrom(sound);
    }

    return ret;
}

BE_NAMESPACE_END
