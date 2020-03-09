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
#include "RHI/RHIOpenGL.h"

BE_NAMESPACE_BEGIN

#ifdef ENABLE_PROFILER
    #define BE_PROFILE_INIT() BE1::profiler.Init()
    #define BE_PROFILE_SHUTDOWN() BE1::profiler.Shutdown()
    #define BE_PROFILE_SYNC_FRAME() BE1::profiler.SyncFrame()
    #define BE_PROFILE_STOP() BE1::profiler.IsFrozen() ? 0 : BE1::profiler.ToggleFreeze();
    #define BE_PROFILE_START() BE1::profiler.IsFrozen() ? BE1::profiler.ToggleFreeze() : 0;
    #define BE_SCOPE_PROFILE_CPU(name, color) static int BE_CONCAT2(tag_cpu_, __LINE__) = BE1::profiler.CreateTag(name, color); BE1::ScopeProfileCPU BE_CONCAT2(profile_scope_cpu_, __LINE__)(BE_CONCAT2(tag_cpu_, __LINE__))
    #define BE_SCOPE_PROFILE_GPU(name, color) static int BE_CONCAT2(tag_gpu_, __LINE__) = BE1::profiler.CreateTag(name, color); BE1::ScopeProfileGPU BE_CONCAT2(profile_scope_gpu_, __LINE__)(BE_CONCAT2(tag_gpu_, __LINE__))
#else
    #define BE_PROFILE_INIT()
    #define BE_PROFILE_SHUTDOWN()
    #define BE_PROFILE_SYNC_FRAME()
    #define BE_PROFILE_STOP()
    #define BE_PROFILE_START()
    #define BE_SCOPE_PROFILE_CPU(name, color)
    #define BE_SCOPE_PROFILE_GPU(name, color)
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
            WaitingForFreeze,
            WaitingForUnfreeze
        };
    };

    struct Tag {
        char                    name[MaxTagNameLength];
        Color3                  color;
    };

    struct MarkerBase {
        int                     tagIndex;
        int                     depth;          // marker stack depth
        int                     frameCount;
    };

    struct CpuMarker : public MarkerBase {
        uint64_t                startTime;
        uint64_t                endTime;
    };

    struct GpuMarker : public MarkerBase {
        RHI::Handle             startQueryHandle;
        RHI::Handle             endQueryHandle;
    };

    struct CpuThreadInfo {
        uint64_t                threadId;
        CpuMarker               markers[MaxCpuMarkersPerThread];
        int                     currentIndex;                       // current marker index
        Stack<int>              indexStack;                         // marker index stack for recursive usage
        int                     frameIndexes[MaxRecordedFrames];    // start marker indexes for frames

        CpuThreadInfo() : indexStack(MaxDepth), currentIndex(0) {}
    };

    struct GpuThreadInfo {
        GpuMarker               markers[MaxGpuMarkers];
        int                     currentIndex;                       // current marker index
        Stack<int>              indexStack;                         // marker index stack for recursive usage
        int                     frameIndexes[MaxRecordedFrames];    // start marker indexes for frames

        GpuThreadInfo() : indexStack(MaxDepth), currentIndex(0) {}
    };

    struct FrameData {
        int                     frameCount;
        uint64_t                time;
    };

    void                        Init();
    void                        Shutdown();

                                // Call SyncFrame() on the starting frame.
    void                        SyncFrame();

    bool                        IsFrozen() const;

    bool                        ToggleFreeze();

    template <typename Func>
    void                        IterateCpuMarkers(uint64_t threadId, Func func) const;

    template <typename Func>
    void                        IterateGpuMarkers(Func func) const;

    int                         CreateTag(const char *name, const Color3 &color);

    void                        PushCpuMarker(int tagIndex);
    void                        PopCpuMarker();

    void                        PushGpuMarker(int tagIndex);
    void                        PopGpuMarker();

private:
    CpuThreadInfo &             GetCpuThreadInfo();

    FreezeState::Enum           freezeState;

    FrameData                   frameData[MaxRecordedFrames];
    int                         frameCount;                     ///< Incremental value for each SyncFrame() calls.
    int                         writeFrameIndex;
    int                         readFrameIndex;

    StaticArray<Tag, MaxTags>   tags;                           ///< tag informations.
    HashMap<uint64_t, CpuThreadInfo> cpuThreadInfoMap;
    GpuThreadInfo               gpuThreadInfo;
};

template <typename Func>
BE_INLINE void Profiler::IterateCpuMarkers(uint64_t threadId, Func func) const {
    if (readFrameIndex < 0) {
        return;
    }

    const FrameData &readFrame = frameData[readFrameIndex];

    if (readFrame.time != InvalidTime) {
        for (int i = 0; i < cpuThreadInfoMap.Count(); i++) {
            const CpuThreadInfo &ti = cpuThreadInfoMap.GetByIndex(i)->second;
            if (ti.threadId != threadId) {
                continue;
            }

            int startMarkerIndex = ti.frameIndexes[readFrameIndex];
            int endMarkerIndex = ti.frameIndexes[(readFrameIndex + 1) % COUNT_OF(ti.frameIndexes)];

            int markerIndex = startMarkerIndex;

            int skipMinDepth = INT_MAX;

            while (markerIndex != endMarkerIndex) {
                const CpuMarker &marker = ti.markers[markerIndex];

                int nextMarkerIndex = (markerIndex + 1) % COUNT_OF(ti.markers);
                bool isLeaf = nextMarkerIndex == endMarkerIndex || ti.markers[nextMarkerIndex].depth <= marker.depth;

                if (marker.depth < skipMinDepth) {
                    const Tag &tag = tags[marker.tagIndex];

                    if (!func(readFrame.frameCount, tag.name, tag.color, marker.depth, isLeaf, marker.startTime, marker.endTime)) {
                        skipMinDepth = marker.depth + 1;
                    } else {
                        skipMinDepth = INT_MAX;
                    }
                }

                markerIndex = nextMarkerIndex;
            }
        }
    }
}

template <typename Func>
BE_INLINE void Profiler::IterateGpuMarkers(Func func) const {
    if (readFrameIndex < 0) {
        return;
    }

    const FrameData &readFrame = frameData[readFrameIndex];

    if (readFrame.time != InvalidTime) {
        const GpuThreadInfo &ti = gpuThreadInfo;

        int startMarkerIndex = ti.frameIndexes[readFrameIndex];
        int endMarkerIndex = ti.frameIndexes[(readFrameIndex + 1) % COUNT_OF(ti.frameIndexes)];

        int markerIndex = startMarkerIndex;

        int skipMinDepth = INT_MAX;

        while (markerIndex != endMarkerIndex) {
            const GpuMarker &marker = ti.markers[markerIndex];

            int nextMarkerIndex = (markerIndex + 1) % COUNT_OF(ti.markers);
            bool isLeaf = nextMarkerIndex == endMarkerIndex || ti.markers[nextMarkerIndex].depth <= marker.depth;

            if (marker.depth < skipMinDepth) {
                while (!rhi.QueryResultAvailable(marker.startQueryHandle)) {}
                while (!rhi.QueryResultAvailable(marker.endQueryHandle)) {}
                uint64_t startTime = rhi.QueryResult(marker.startQueryHandle);
                uint64_t endTime = rhi.QueryResult(marker.endQueryHandle);

                const Tag &tag = tags[marker.tagIndex];

                if (!func(readFrame.frameCount, tag.name, tag.color, marker.depth, isLeaf, startTime, endTime)) {
                    skipMinDepth = marker.depth + 1;
                } else {
                    skipMinDepth = INT_MAX;
                }
            }

            markerIndex = (markerIndex + 1) % COUNT_OF(ti.markers);
        }
    }
}

extern Profiler     profiler;

class ScopeProfileCPU {
public:
    ScopeProfileCPU(int tagIndex) { profiler.PushCpuMarker(tagIndex); }
    ~ScopeProfileCPU() { profiler.PopCpuMarker(); }
};

class ScopeProfileGPU {
public:
    ScopeProfileGPU(int tagIndex) { profiler.PushGpuMarker(tagIndex); }
    ~ScopeProfileGPU() { profiler.PopGpuMarker(); }
};

BE_NAMESPACE_END
