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

#pragma once

#include "Math/Math.h"
#include "Core/Str.h"
#include "Containers/LinkList.h"
#include "Containers/HashMap.h"
#include "Core/CVars.h"
#include "Sound/Pcm.h"

#if defined(__ANDROID__)

// Reference: https://googlesamples.github.io/android-audio-high-performance/guides/opensl_es.html
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

#elif defined(__APPLE__)

#include <OpenAL/al.h>
#include <OpenAL/alc.h>

#elif defined(__WIN32__)

#ifdef USE_WINDOWS_OPENAL
#include "al.h"
#include "alc.h"

#else

#include <mmsystem.h>
#include <dsound.h>
#endif

#endif

BE_NAMESPACE_BEGIN

class CmdArgs;
class SoundBuffer;
class SoundSource;
class Sound;

class SoundBuffer {
public:
    static const int        MaxStreamBuffers = 3;
    static const int        StreamBufferSeconds = 1;

    SoundBuffer();
    ~SoundBuffer();

    void                    Purge();

    bool                    CreateStaticBuffer(int numChannels, int bitsWidth, int sampleRates, int size, const byte *data);

    bool                    CreateStreamBuffer(int numChannels, int bitsWidth, int sampleRates);

    bool                    DuplicateBuffer(SoundBuffer *originalSoundBuffer);

    bool                    duplicated;

#if defined(__ANDROID__)
    uint32_t                bufferSize;
    uint32_t                bufferCount;
    byte *                  buffers[MaxStreamBuffers];
#elif defined(__APPLE__) || (defined(__WIN32__) && defined(USE_WINDOWS_OPENAL))
    ALuint                  alBufferIds[MaxStreamBuffers];
    uint32_t                bufferCount;
    ALenum                  format;
    ALsizei                 sampleRates;
#elif defined(__WIN32__)
    IDirectSoundBuffer *    dsBuffer;
    IDirectSound3DBuffer *  ds3dBuffer;
    uint32_t                streamBufferSize;
    uint32_t                streamBufferCount;
#endif
};

class SoundSource {
public:
    SoundSource();
    ~SoundSource();

    bool                    IsFinished();

    void                    Init(Sound *sound);

    void                    Update();

    void                    UpdateStream();

    bool                    Stop();
    void                    Pause();
    void                    Play();

    uint32_t                GetCurrentOffset() const;
    void                    SetCurrentOffset(uint32_t offset);

    void                    SetVolume(float volume);

    Sound *                 sound;
    Pcm                     pcm;    ///< PCM file for streaming

#if defined(__ANDROID__)
    bool                    CreateAudioPlayer(const Sound *sound);
    void                    DestroyAudioPlayer();
    void                    OnRequeueBufferCallback(SLAndroidSimpleBufferQueueItf bufferQueue);

    SLObjectItf             slPlayerObject;
    SLPlayItf               slPlay;
    SLVolumeItf             slVolume;
    SLSeekItf               slSeek;
    SL3DLocationItf         sl3DLocation;
    SLAndroidSimpleBufferQueueItf slBufferQueue;
    SLuint32                lastBufferQueueIndex;
    SLuint32                bufferUnqueueIndex;
    bool                    hasPositionUpdated;
#elif defined(__APPLE__) || (defined(__WIN32__) && defined(USE_WINDOWS_OPENAL))
    ALuint                  alSourceId;
#elif defined(__WIN32__)
    IDirectSoundBuffer *    dsBuffer;
    IDirectSound3DBuffer *  ds3dBuffer;
    uint32_t                streamWriteOffset;
    bool                    streamEnded;
#endif
};

class Sound {
    friend class SoundSource;
    friend class SoundSystem;

public:
    Sound();
    ~Sound();

    const char *            GetName() const { return name; }
    const char *            GetHashName() const { return hashName; }

    int                     NumChannels() const { return numChannels; }
    int                     SampleRates() const { return sampleRates; }
    int                     BitsWidth() const { return bitsWidth; }
    int                     Duration() const { return duration; }
    int                     Bytes() const { return bytes; }
    int                     ByteOffset() const;

    void                    Purge();

    void                    Create(Pcm &pcm);

    Sound *                 Instantiate();

    void                    Play2D(float volume = 1.0f, bool looping = false);
    void                    Play3D(const Vec3 &origin, float minDistance, float maxDistance, float volume = 1.0f, bool looping = false);
    void                    Stop();

                            /// Is currently playing ?
    bool                    IsPlaying() const;

                            /// Returns volume in [0, 1]
    float                   GetVolume() const { return volume; }
                            /// Sets volume in [0, 1]
    void                    SetVolume(float volume);

                            /// Gets current playing time in milliseconds
    uint32_t                GetPlayingTime() const;
                            /// Sets current playing time in milliseconds
    void                    SetPlayingTime(uint32_t time);

                            /// Sets 3D position of this sound
    void                    UpdatePosition(const Vec3 &origin); 

    bool                    Load(const char *filename);

    bool                    Reload();

private:
    void                    CreateStatic(Pcm &pcm);
    void                    CreateStream(Pcm &pcm);
    void                    DuplicateFrom(Sound *sound);

    Str                     hashName;
    Str                     name;
    mutable int             refCount;
    bool                    permanence;

    bool                    isStream;               ///< True for stream buffer, false for static buffer
    SoundBuffer *           soundBuffer;
    int                     numChannels;
    int                     sampleRates;
    int                     bitsWidth;
    int                     bytes;
    int                     duration;

    int                     playingTime;            ///< Current playing time
    float                   priority;
    float                   volume;
    Vec3                    origin;

    bool                    localSound;             ///< True for 2D sound, false for 3D sound
    bool                    looping;
    float                   minDistance;
    float                   maxDistance;

    SoundSource *           soundSource;

    LinkList<Sound>         playNode;
    LinkList<Sound>         dupNode;
    Sound *                 originalSound;
};

class SoundSystem {
    friend class Sound;
    friend class SoundBuffer;
    friend class SoundSource;

public:
    SoundSystem() { initialized = false; }
    ~SoundSystem() {}

    void                    Init(void *windowHandle = nullptr);
    void                    Shutdown();

    void                    Update();

    Sound *                 AllocSound(const char *name);
    Sound *                 FindSound(const char *name) const;
    Sound *                 GetSound(const char *name);

    void                    ReleaseSound(Sound *sound, bool immediateDestroy = false);
    void                    DestroySound(Sound *sound);
    void                    DestroyAllSounds();
    void                    DestroyUnusedSounds();

    void                    PrecacheSound(const char *filename);

    void                    RenameSound(Sound *sound, const Str &newName);

                            /// Sets where the camera is
    void                    PlaceListener(const Vec3 &pos, const Mat3 &axis);

    void                    StopAllSounds();

    float                   GetMasterVolume() const;
    void                    SetMasterVolume(float volume);

    Sound *                 defaultSound;

    static CVar             s_nosound;
    static CVar             s_volume;

private:
    bool                    InitDevice(void *windowHandle);
    void                    ShutdownDevice();
    void                    PlaceListenerInternal(const Vec3 &origin, const Vec3 &forward, const Vec3 &up);
    void                    CreateDefaultSound();

    static void             Cmd_SoundInfo(const CmdArgs &args);
    static void             Cmd_ListSounds(const CmdArgs &args);
    static void             Cmd_PlaySound(const CmdArgs &args);

    bool                    initialized;

    StrIHashMap<Sound *>    soundHashMap;
    LinkList<Sound>         soundPlayLinkList;

    Array<Sound *>          prioritySounds;

    Array<SoundSource *>    sources;
    Array<SoundSource *>    freeSources;

    Vec3                    listenerPosition;
    Vec3                    listenerForward;
    Vec3                    listenerUp;

#if defined(__ANDROID__)
    SLObjectItf             slEngineObject;
    SLEngineItf             slEngine;
    SLObjectItf             slOutputMixObject;
    SLObjectItf             slListenerObject;
#elif defined(__APPLE__) || (defined(__WIN32__) && defined(USE_WINDOWS_OPENAL))
    ALCcontext *            alContext;
    ALCdevice *             alDevice;
#elif defined(__WIN32__)
    HWND                    hwnd;
    IDirectSound8 *         dsDevice;
    IDirectSoundBuffer *    dsPrimaryBuffer;
    IDirectSound3DListener8 *dsListener3D;
#endif
};

extern SoundSystem          soundSystem;

BE_NAMESPACE_END
