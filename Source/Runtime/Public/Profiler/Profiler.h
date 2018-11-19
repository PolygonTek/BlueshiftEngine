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

#include "Containers/HashMap.h"
#include "Containers/Stack.h"
#include "Math/Math.h"
#include "RHI/RHI.h"

BE_NAMESPACE_BEGIN

#define ENABLE_PROFILER

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
#define BE_PROFILE_CPU_SCOPE(name, color) ProfileCpuScope profile_##__LINE__(name, color)
#define BE_PROFILE_GPU_SCOPE(name, color) ProfileGpuScope profile_##__LINE__(name, color)

#endif

class Profiler {
public:
    static const int        MaxRecordedFrames = 4;
    static const int        MaxCpuThreads = 32;
    static const int        MaxDepth = 32;
    static const uint64_t   InvalidTime = -1;

    static const int        MaxCpuMarkersPerFrame = 100;
    static const int        MaxCpuMarkersPerThread = MaxRecordedFrames * MaxCpuMarkersPerFrame;

    static const int        MaxGpuMarkersPerFrame = 10;
    static const int        MaxGpuMarkers = MaxRecordedFrames * MaxGpuMarkersPerFrame;

    static const int        MaxMarkerNameLength = 32;

    struct Marker {
        char                name[MaxMarkerNameLength];
        Color3              color;
        int                 depth;
        int                 frameCount;
    };

    struct CpuMarker : public Marker {
        uint64_t            startTime;
        uint64_t            endTime;
    };

    struct GpuMarker : public Marker {
        RHI::Handle         startQueryHandle;
        RHI::Handle         endQueryHandle;
    };

    struct CpuThreadInfo {
        uint64_t            threadId;
        CpuMarker           markers[MaxCpuMarkersPerThread];
        Stack<int>          markerIndexStack;
        int                 writeMarkerIndex;

        CpuThreadInfo() : markerIndexStack(MaxDepth) {}
    };

    struct GpuThreadInfo {
        GpuMarker           markers[MaxGpuMarkers];
        Stack<int>          markerIndexStack;
        int                 writeMarkerIndex;

        GpuThreadInfo() : markerIndexStack(MaxDepth) {}
    };

    void                    Init();
    void                    Shutdown();

    void                    SyncFrame();

    void                    PushCpuMarker(const char *name, const Color3 &color);
    void                    PopCpuMarker();

    void                    PushGpuMarker(const char *name, const Color3 &color);
    void                    PopGpuMarker();

private:
    CpuThreadInfo &         GetCpuThreadInfo();

    int                     frameCount;

    HashMap<uint64_t, CpuThreadInfo> cpuThreadInfoMap;

    GpuThreadInfo           gpuThreadInfo;
};

extern Profiler             profiler;

class ProfileCpuScope {
public:
    ProfileCpuScope(const char *name, const Color3 &color) { profiler.PushCpuMarker(name, color); }
    ~ProfileCpuScope() { profiler.PopCpuMarker(); }
};

class ProfileGpuScope {
public:
    ProfileGpuScope(const char *name, const Color3 &color) { profiler.PushGpuMarker(name, color); }
    ~ProfileGpuScope() { profiler.PopGpuMarker(); }
};

BE_NAMESPACE_END
