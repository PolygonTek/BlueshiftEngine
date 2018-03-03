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

SoundSystem     soundSystem;

CVar            SoundSystem::s_nosound(L"s_nosound", L"0", 0, L"");
CVar            SoundSystem::s_volume(L"s_volume", L"0.8", CVar::Float | CVar::Archive, L"");

void SoundSystem::Init(void *windowHandle) {
    BE_LOG(L"Initializing SoundSystem...\n");

    if (s_nosound.GetBool()) {
        BE_LOG(L"s_nosound is on, sound system cannot be initialized.\n");
        return; // FIXME
    }

    cmdSystem.AddCommand(L"listSounds", Cmd_ListSounds);
    cmdSystem.AddCommand(L"playSound", Cmd_PlaySound);
 
    if (!InitDevice(windowHandle)) {
        return;
    }    

    CreateDefaultSound();

    // Sounds to be sorted by priority
    prioritySounds.Resize(64, 64);

    listenerPosition = Vec3::zero;
    listenerForward = Vec3::unitX;
    listenerUp = Vec3::unitZ;

    initialized = true;
}

void SoundSystem::Shutdown() {
    if (!initialized) {
        return;
    }
     
    cmdSystem.RemoveCommand(L"listSounds");
    cmdSystem.RemoveCommand(L"playSound");
 
    DestroyAllSounds();

    soundPlayLinkList.Clear();

    ShutdownDevice();

    initialized = false;
}

void SoundSystem::DestroyAllSounds() {
    LinkList<Sound> *nextNode;

    for (int i = 0; i < soundHashMap.Count(); i++) {
        const auto *entry = soundHashMap.GetByIndex(i);
        Sound *sound = entry->second;

        if (!sound->dupNode.IsListEmpty()) {
            for (LinkList<Sound> *node = sound->dupNode.NextNode(); node; node = nextNode) {
                nextNode = node->NextNode();

                Sound *s = node->Owner();
                delete s;
            }

            sound->dupNode.Clear();
        }

        delete sound;
    }

    soundHashMap.Clear();
}

void SoundSystem::DestroyUnusedSounds() {
    Array<Sound *> removeArray;

    for (int i = 0; i < soundHashMap.Count(); i++) {
        const auto *entry = soundHashMap.GetByIndex(i);
        Sound *sound = entry->second;

        if (sound && !sound->permanence && sound->refCount == 0) {
            removeArray.Append(sound);
        }
    }

    for (int i = 0; i < removeArray.Count(); i++) {
        Sound *sound = removeArray[i];

        if (!sound->dupNode.IsListEmpty()) {
            LinkList<Sound> *nextNode;

            for (LinkList<Sound> *node = sound->dupNode.NextNode(); node; node = nextNode) {
                nextNode = node->NextNode();

                Sound *s = node->Owner();
                delete s;
            }

            sound->dupNode.Clear();
        }

        soundHashMap.Remove(sound->hashName);

        delete sound;
    }
}

void SoundSystem::CreateDefaultSound() {
    const float waveFrequency = 64;
    const int sampleRates = 22050;
    const int bitsWidth = 8;

    Pcm pcm;
    pcm.Create(1, sampleRates, bitsWidth, sizeof(int8_t) * 4096, nullptr);
    int8_t *data = (int8_t *)pcm.GetData();

    for (int i = 0; i < 2048; i++) {
        data[i] = Math::Sin(waveFrequency * Math::TwoPi * i / sampleRates) * (1 << (bitsWidth - 2));
    }
    memset(data + 2048, 0, 2048);

    defaultSound = AllocSound("_defaultSound");
    defaultSound->permanence = true;
    defaultSound->Create(pcm);
}

void SoundSystem::PlaceListener(const Vec3 &pos, const Mat3 &axis) {
    listenerPosition = pos;
    listenerForward = axis[0];
    listenerUp = axis[2];
}

void SoundSystem::PrecacheSound(const char *filename) {
    Sound *sound = GetSound(filename);
    ReleaseSound(sound);
}

void SoundSystem::StopAllSounds() {
    for (int sourceIndex = 0; sourceIndex < sources.Count(); sourceIndex++) {
        SoundSource *source = sources[sourceIndex];

        if (source->sound) {
            if (source->Stop()) {
                source->sound->playNode.Remove();
                source->sound->soundSource = nullptr;
                source->sound = nullptr;

                freeSources[sourceIndex] = source;
            }
        }
    }
}

float SoundSystem::GetMasterVolume() const {
    return s_volume.GetFloat();
}

void SoundSystem::SetMasterVolume(float volume) {
    s_volume.SetFloat(volume);
}

Sound *SoundSystem::AllocSound(const char *hashName) {
    if (soundHashMap.Get(hashName)) {
        BE_FATALERROR(L"'%hs' sound buffer already allocated", hashName);
    }

    Sound *sound = new Sound;
    sound->refCount = 1;
    sound->hashName = hashName;
    sound->name = hashName;
    sound->name.StripPath();
    sound->name.StripFileExtension();
    sound->dupNode.SetOwner(sound);

    soundHashMap.Set(sound->hashName, sound);

    return sound;
}

void SoundSystem::RenameSound(Sound *sound, const Str &newName) {
    const auto *entry = soundHashMap.Get(sound->hashName);
    if (entry) {
        soundHashMap.Remove(sound->hashName);

        sound->hashName = newName;
        sound->name = newName;
        sound->name.StripPath();
        sound->name.StripFileExtension();

        soundHashMap.Set(newName, sound);
    }
}

void SoundSystem::DestroySound(Sound *sound) {
    if (sound->originalSound) {
        delete sound;
        return;
    }

    if (sound->refCount > 1) {
        BE_WARNLOG(L"SoundSystem::DestroySound: sound '%hs' has %i reference count\n", sound->name.c_str(), sound->refCount);
    }

    soundHashMap.Remove(sound->hashName);
    delete sound;
}

void SoundSystem::ReleaseSound(Sound *sound, bool immediateDestroy) {
    if (sound->permanence) {
        return;
    }

    if (sound->originalSound) {
        if (immediateDestroy) {
            delete sound;
        }
        return;
    }

    if (sound->refCount > 0) {
        sound->refCount--;
    }

    if (immediateDestroy && sound->refCount == 0) {
        DestroySound(sound);
    }
}

Sound *SoundSystem::FindSound(const char *hashName) const {
    const auto *entry = soundHashMap.Get(Str(hashName));
    if (entry) {
        return entry->second;
    }

    return nullptr;
}

Sound *SoundSystem::GetSound(const char *hashName) {
    if (!initialized) {
        return nullptr;
    }

    Sound *sound = FindSound(hashName);
    if (sound) {
        sound->refCount++;
        return sound;
    }

    sound = AllocSound(hashName);
    if (!sound->Load(hashName)) {
        BE_WARNLOG(L"Couldn't load sound '%hs'\n", hashName);
        DestroySound(sound);
        return defaultSound;
    }

    return sound;
}

// TODO: SoundSystem::Update 함수를 별도 쓰레드로 바꿀것
void SoundSystem::Update() {
    static float lastTime = PlatformTime::Milliseconds();
    LinkList<Sound> *node;
    LinkList<Sound> *nextNode;

    if (!initialized) {
        return;
    }

    int currentTime = PlatformTime::Milliseconds();
    int elapsedTime = currentTime - lastTime;
    lastTime = currentTime;

    // Stop any sources that have finished
    for (int sourceIndex = 0; sourceIndex < sources.Count(); sourceIndex++) {
        SoundSource *source = sources[sourceIndex];

        if (source->IsFinished()) {
            if (source->Stop()) {
                // Remove from the play list
                source->sound->playNode.Remove();
                source->sound->soundSource = nullptr;
                source->sound = nullptr;

                freeSources[sourceIndex] = source;
            }
        }
    }

    prioritySounds.SetCount(0);

    for (node = soundPlayLinkList.NextNode(); node; node = nextNode) {
        nextNode = node->NextNode();

        Sound *sound = node->Owner();
        
        if (sound->localSound) {
            sound->priority = sound->volume * (sound->looping ? 1.0f : (1.0f - (float)sound->playingTime / sound->duration));
        } else {
            float fallOff = listenerPosition.DistanceSqr(sound->origin) / (sound->maxDistance * sound->maxDistance);
            sound->priority = 1.0f - Min(fallOff, 1.0f);
        }        

        prioritySounds.Append(sound);
    }

    // Sort sounds by priority decending order
    prioritySounds.Sort([](const Sound *a, const Sound *b) {
        return a->priority - b->priority > 0;
    });

    // Stop sources that have lower priority first
    int soundIndex = prioritySounds.Count() - 1;
    for (; soundIndex >= 0; soundIndex--) {
        Sound *playSound = prioritySounds[soundIndex];

        if (soundIndex >= sources.Count() || playSound->priority <= 0) {
            if (playSound->soundSource) {
                int sourceIndex = sources.FindIndex(playSound->soundSource);
                assert(sourceIndex >= 0);

                freeSources[sourceIndex] = playSound->soundSource;

                playSound->soundSource->Stop();
                playSound->soundSource->sound = nullptr;
                playSound->soundSource = nullptr;
            }
        } else {
            break;
        }
    }

    // Play sources
    for (; soundIndex >= 0; soundIndex--) {
        Sound *playSound = prioritySounds[soundIndex];

        if (playSound->soundSource) {
            playSound->soundSource->Update();
        } else {
            int sourceIndex = freeSources.FindNotNull();
            assert(sourceIndex >= 0);

            playSound->soundSource = freeSources[sourceIndex];
            playSound->soundSource->Init(playSound);
            playSound->soundSource->Play();

            freeSources[sourceIndex] = nullptr;
        }
    }

    // Update playing time of each playing sounds
    for (node = soundPlayLinkList.NextNode(); node; node = nextNode) {
        nextNode = node->NextNode();

        Sound *sound = node->Owner();

        sound->playingTime += elapsedTime;
        if (sound->playingTime > sound->duration) {
            if (sound->looping) {
                while (sound->playingTime >= sound->duration) {
                    sound->playingTime -= sound->duration;
                }
            } else {
                sound->playingTime = sound->duration;
            }
        }
    }

    // Update system volume if it is modified by cvar
    if (s_volume.IsModified()) {
        s_volume.ClearModified();

        for (int soundIndex = 0; soundIndex < Min(sources.Count(), prioritySounds.Count()); soundIndex++) {
            Sound *playSound = prioritySounds[soundIndex];

            playSound->SetVolume(playSound->volume);
        }
    }

    PlaceListenerInternal(listenerPosition, listenerForward, listenerUp);
}

//--------------------------------------------------------------------------------------------------

void SoundSystem::Cmd_ListSounds(const CmdArgs &args) {
    int count = 0;

    for (int i = 0; i < soundSystem.soundHashMap.Count(); i++) {
        const auto *entry = soundSystem.soundHashMap.GetByIndex(i);
        Sound *sound = entry->second;

        if (!sound) {
            continue;
        }

        int channels = sound->NumChannels();
        int samples = sound->SampleRates();
        int bits = sound->BitsWidth();

        BE_LOG(L"%3d refs %6hs %5i khz %2i bits %hs %hs\n",
            sound->refCount, channels > 1 ? "stereo" : "mono", samples, bits, Str::FormatBytes(sound->Bytes()).c_str(), sound->hashName.c_str());

        count++;
    }
    BE_LOG(L"%i sounds loaded\n", count);
}

void SoundSystem::Cmd_PlaySound(const CmdArgs &args) {
    if (args.Argc() != 2) {
        BE_LOG(L"usage: playsound <sound-file>\n");
        return;
    }

    Str filename = WStr::ToStr(args.Argv(1));

    Sound *sound = soundSystem.GetSound(filename);
    Sound *s = sound->Instantiate();
    s->Play2D();
    soundSystem.ReleaseSound(sound);
}

BE_NAMESPACE_END
