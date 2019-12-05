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

#include "Containers/StaticArray.h"
#include "Containers/HashMap.h"
#include "Containers/Stack.h"
#include "Math/Math.h"
#include "RHI/RHI.h"

BE_NAMESPACE_BEGIN

#ifdef DEVELOPMENT
#define ENABLE_PROFILER
#endif

#ifndef ENABLE_PROFILER

#define BE_PROFILE_INIT()
#define BE_PROFILE_SHUTDOWN()
#define BE_PROFILE_SYNC_FRAME()
#define BE_PROFILE_CPU_SCOPE(name, color)
#define BE_PROFILE_GPU_SCOPE(name, color)

#else

#define BE_PROFILE_INIT() profiler.Init()
#define BE_PROFILE_SHUTDOWN() profiler.Shutdown()
#define BE_PROFILE_SYNC_FRAME() profiler.SyncFrame()
#define BE_PROFILE_CPU_SCOPE(name, color) static int tag_##__LINE__ = profiler.CreateTag(name, color); ProfileCpuScope profile_scope_##__LINE__(tag_##__LINE__)
#define BE_PROFILE_GPU_SCOPE(name, color) static int tag_##__LINE__ = profiler.CreateTag(name, color); ProfileGpuScope profile_scope_##__LINE__(tag_##__LINE__)

#endif

class Profiler {
public:
    static const int            MaxRecordedFrames = 3;
    static const int            MaxTags = 1024;
    static const int            MaxCpuThreads = 32;
    static const int            MaxDepth = 32;

    static const uint64_t       InvalidTime = -1;

    static const int            MaxCpuMarkersPerFrame = 100;
    static const int            MaxCpuMarkersPerThread = MaxRecordedFrames * MaxCpuMarkersPerFrame;

    static const int            MaxGpuMarkersPerFrame = 100;
    static const int            MaxGpuMarkers = MaxRecordedFrames * MaxGpuMarkersPerFrame;

    static const int            MaxTagNameLength = 64;

    struct FreezeState {
        enum Enum {
            Unfrozen,
            Frozen,
            WatingForFreeze,
            WatingForUnfreeze
        };
    };

    struct Tag {
        char                    name[MaxTagNameLength];
        Color3                  color;
    };

    struct Marker {
        int                     tagIndex;
        int                     depth;
        int                     frameCount;
    };

    struct CpuMarker : public Marker {
        uint64_t                startTime;
        uint64_t                endTime;
    };

    struct GpuMarker : public Marker {
        RHI::Handle             startQueryHandle;
        RHI::Handle             endQueryHandle;
    };

    struct CpuThreadInfo {
        uint64_t                threadId;
        CpuMarker               markers[MaxCpuMarkersPerThread];
        Stack<int>              markerIndexStack;
        int                     writeMarkerIndex;
        int                     frameMarkerIndexes[MaxRecordedFrames];

        CpuThreadInfo() : markerIndexStack(MaxDepth), writeMarkerIndex(0) {}
    };

    struct GpuThreadInfo {
        GpuMarker               markers[MaxGpuMarkers];
        Stack<int>              markerIndexStack;
        int                     writeMarkerIndex;
        int                     frameMarkerIndexes[MaxRecordedFrames];

        GpuThreadInfo() : markerIndexStack(MaxDepth), writeMarkerIndex(0) {}
    };

    struct FrameData {
        int                     frameCount;
        uint64_t                time;
    };

    void                        Init();
    void                        Shutdown();

    void                        SyncFrame();

    bool                        IsFrozen() const { return (freezeState == FreezeState::Frozen || freezeState == FreezeState::WatingForUnfreeze); }
    bool                        ToggleFreeze();

    int                         CreateTag(const char *name, const Color3 &color);

    void                        PushCpuMarker(int tagIndex);
    void                        PopCpuMarker();

    void                        PushGpuMarker(int tagIndex);
    void                        PopGpuMarker();

private:
    CpuThreadInfo &             GetCpuThreadInfo();

    FreezeState::Enum           freezeState;

    int                         frameCount;
    int                         currentFrameDataIndex;
    int                         readFameDataIndex;
    FrameData                   frameData[MaxRecordedFrames];

    StaticArray<Tag, MaxTags>   tags;
    HashMap<uint64_t, CpuThreadInfo> cpuThreadInfoMap;
    GpuThreadInfo               gpuThreadInfo;
};

extern Profiler                 profiler;

class ProfileCpuScope {
public:
    ProfileCpuScope(int tagIndex) { profiler.PushCpuMarker(tagIndex); }
    ~ProfileCpuScope() { profiler.PopCpuMarker(); }
};

class ProfileGpuScope {
public:
    ProfileGpuScope(int tagIndex) { profiler.PushGpuMarker(tagIndex); }
    ~ProfileGpuScope() { profiler.PopGpuMarker(); }
};

BE_NAMESPACE_END
