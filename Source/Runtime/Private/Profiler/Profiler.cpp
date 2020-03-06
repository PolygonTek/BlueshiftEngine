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
#include "Profiler/Profiler.h"
#include "Platform/PlatformThread.h"
#include "Platform/PlatformTime.h"
#include "RHI/RHIOpenGL.h"

BE_NAMESPACE_BEGIN

Profiler profiler;

static PlatformMutex *mapMutex;

void Profiler::Init() {
    freezeState = FreezeState::Unfrozen;

    frameCount = 0;
    writeFrameIndex = 0;
    readFrameIndex = -1;

    for (int i = 0; i < COUNT_OF(frameData); i++) {
        FrameData &fd = frameData[i];

        fd.frameCount = 0;
        fd.time = InvalidTime;
    }

    cpuThreadInfoMap.Init(MaxCpuThreads, MaxCpuThreads, MaxCpuThreads);

    // Create GPU queries for GPU markers.
    for (int i = 0; i < COUNT_OF(gpuThreadInfo.markers); i++) {
        GpuMarker &marker = gpuThreadInfo.markers[i];

        marker.startQueryHandle = rhi.CreateQuery(RHI::QueryType::Timestamp);
        marker.endQueryHandle = rhi.CreateQuery(RHI::QueryType::Timestamp);
    }

    mapMutex = (PlatformMutex *)PlatformMutex::Create();
}

void Profiler::Shutdown() {
    // Destroy all GPU queries.
    for (int i = 0; i < COUNT_OF(gpuThreadInfo.markers); i++) {
        auto &marker = gpuThreadInfo.markers[i];

        rhi.DestroyQuery(marker.startQueryHandle);
        rhi.DestroyQuery(marker.endQueryHandle);
    }

    PlatformMutex::Destroy(mapMutex);
}

void Profiler::SyncFrame() {
    if (freezeState == FreezeState::WaitingForFreeze) {
        freezeState = FreezeState::Frozen;
    } else if (freezeState == FreezeState::WaitingForUnfreeze) {
        freezeState = FreezeState::Unfrozen;
    }

    if (IsFrozen()) {
        return;
    }

    readFrameIndex = (writeFrameIndex + 1) % COUNT_OF(frameData);

    const FrameData &readFrame = frameData[readFrameIndex];

    if (readFrame.time != InvalidTime) {
        for (int i = 0; i < cpuThreadInfoMap.Count(); i++) {
            const CpuThreadInfo &ti = cpuThreadInfoMap.GetByIndex(i)->second;

            int startMarkerIndex = ti.frameIndexes[readFrameIndex];
            int endMarkerIndex = ti.frameIndexes[(readFrameIndex + 1) % COUNT_OF(ti.frameIndexes)];

            for (int markerIndex = startMarkerIndex; markerIndex < endMarkerIndex; markerIndex++) {
                const CpuMarker &marker = ti.markers[markerIndex];

                BE_LOG("%s: %.3f\n", tags[marker.tagIndex].name, (marker.endTime - marker.startTime) / 1000000.0f);
            }
        }
    }

    writeFrameIndex = frameCount % COUNT_OF(frameData);

    FrameData &writeFrame = frameData[writeFrameIndex];

    writeFrame.frameCount = frameCount;
    writeFrame.time = PlatformTime::Microseconds();

    for (int i = 0; i < cpuThreadInfoMap.Count(); i++) {
        CpuThreadInfo &ti = cpuThreadInfoMap.GetByIndex(i)->second;

        ti.frameIndexes[writeFrameIndex] = ti.currentIndex;
    }

    gpuThreadInfo.frameIndexes[writeFrameIndex] = gpuThreadInfo.currentIndex;

    frameCount++;
}

bool Profiler::ToggleFreeze() {
    if (freezeState == FreezeState::Unfrozen) {
        // Arise freeze in the next SyncFrame() call.
        freezeState = FreezeState::WaitingForFreeze;
        return true;
    }
    if (freezeState == FreezeState::Frozen) {
        // Arise unfreeze in the next SyncFrame() call.
        freezeState = FreezeState::WaitingForUnfreeze;
        return true;
    }
    return false;
}

int Profiler::CreateTag(const char *name, const Color3 &color) {
    Tag tag;
    Str::Copynz(tag.name, name, COUNT_OF(tag.name));
    tag.color = color;

    int tagIndex = tags.Append(tag);

    return tagIndex;
}

Profiler::CpuThreadInfo &Profiler::GetCpuThreadInfo() {
    uint64_t tid = PlatformThread::GetCurrentThreadId();

    auto *entry = cpuThreadInfoMap.Get(tid);
    if (entry) {
        return entry->second;
    }

    PlatformMutex::Lock(mapMutex);

    CpuThreadInfo cpuThreadInfo;
    cpuThreadInfo.threadId = tid;
    cpuThreadInfo.frameIndexes[writeFrameIndex] = cpuThreadInfo.currentIndex;

    entry = cpuThreadInfoMap.Set(tid, cpuThreadInfo);

    PlatformMutex::Unlock(mapMutex);

    return entry->second;
}

void Profiler::PushCpuMarker(int tagIndex) {
    if (IsFrozen()) {
        return;
    }

    CpuThreadInfo &ti = GetCpuThreadInfo();
    CpuMarker &marker = ti.markers[ti.currentIndex];

    ti.currentIndexStack.Push(ti.currentIndex);
    ti.currentIndex = (ti.currentIndex + 1) % COUNT_OF(ti.markers);

    marker.tagIndex = tagIndex;
    marker.startTime = PlatformTime::Microseconds();
    marker.endTime = InvalidTime;
    marker.frameCount = frameCount;
    marker.depth = ti.currentIndexStack.Count();
}

void Profiler::PopCpuMarker() {
    if (IsFrozen()) {
        return;
    }

    CpuThreadInfo &ti = GetCpuThreadInfo();
    assert(!ti.currentIndexStack.IsEmpty());

    int currentIndex = ti.currentIndexStack.Pop();

    CpuMarker &marker = ti.markers[currentIndex];

    marker.endTime = PlatformTime::Microseconds();
}

void Profiler::PushGpuMarker(int tagIndex) {
    if (IsFrozen()) {
        return;
    }

    GpuThreadInfo &ti = gpuThreadInfo;
    GpuMarker &marker = ti.markers[ti.currentIndex];

    ti.currentIndexStack.Push(ti.currentIndex);
    ti.currentIndex = (ti.currentIndex + 1) % COUNT_OF(ti.markers);

    marker.tagIndex = tagIndex;
    rhi.QueryTimestamp(marker.startQueryHandle);
    marker.frameCount = frameCount;
    marker.depth = ti.currentIndexStack.Count();
}

void Profiler::PopGpuMarker() {
    if (IsFrozen()) {
        return;
    }

    GpuThreadInfo &ti = gpuThreadInfo;
    assert(!ti.currentIndexStack.IsEmpty());

    int currentIndex = ti.currentIndexStack.Pop();

    GpuMarker &marker = ti.markers[currentIndex];

    rhi.QueryTimestamp(marker.endQueryHandle);
}

BE_NAMESPACE_END
