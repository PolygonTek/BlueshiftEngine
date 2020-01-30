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

static PlatformMutex *mapAddMutex;

void Profiler::Init() {
    freezeState = FreezeState::Unfrozen;

    frameCount = 0;
    currentFrameDataIndex = 0;
    readFameDataIndex = -1;

    for (int i = 0; i < COUNT_OF(frameData); i++) {
        auto &fd = frameData[i];

        fd.time = InvalidTime;
    }

    cpuThreadInfoMap.Init(MaxCpuThreads, MaxCpuThreads, MaxCpuThreads);

    for (int i = 0; i < COUNT_OF(gpuThreadInfo.markers); i++) {
        auto &marker = gpuThreadInfo.markers[i];

        marker.startQueryHandle = rhi.CreateQuery(RHI::QueryType::Timestamp);
        marker.endQueryHandle = rhi.CreateQuery(RHI::QueryType::Timestamp);
    }

    mapAddMutex = (PlatformMutex *)PlatformMutex::Create();
}

void Profiler::Shutdown() {
    for (int i = 0; i < COUNT_OF(gpuThreadInfo.markers); i++) {
        auto &marker = gpuThreadInfo.markers[i];

        rhi.DestroyQuery(marker.startQueryHandle);
        rhi.DestroyQuery(marker.endQueryHandle);
    }

    PlatformMutex::Destroy(mapAddMutex);
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

    readFameDataIndex = (currentFrameDataIndex + 1) % COUNT_OF(frameData);

    // readFrameDataIndex is valid only if frameData[readFrameDataIndex].time != InvalidTime
    if (frameData[readFameDataIndex].time != InvalidTime) {
        
    }

    currentFrameDataIndex = frameCount % COUNT_OF(frameData);

    FrameData &currentFrame = frameData[currentFrameDataIndex];

    currentFrame.frameCount = frameCount;
    currentFrame.time = PlatformTime::Nanoseconds();

    for (int i = 0; i < cpuThreadInfoMap.Count(); i++) {
        CpuThreadInfo &ti = cpuThreadInfoMap.GetByIndex(i)->second;

        ti.frameMarkerIndexes[currentFrameDataIndex] = ti.writeMarkerIndex;
    }

    gpuThreadInfo.frameMarkerIndexes[currentFrameDataIndex] = gpuThreadInfo.writeMarkerIndex;

    frameCount++;
}

bool Profiler::ToggleFreeze() {
    if (freezeState == FreezeState::Unfrozen) {
        freezeState = FreezeState::WaitingForFreeze;
        return true;
    }
    if (freezeState == FreezeState::Frozen) {
        freezeState = FreezeState::WaitingForUnfreeze;
        return true;
    }
    return false;
}

int Profiler::CreateTag(const char *name, const Color3 &color) {
    Tag tag;
    Str::Copynz(tag.name, name, COUNT_OF(tag.name));
    tag.color = color;

    return tags.Append(tag);
}

Profiler::CpuThreadInfo &Profiler::GetCpuThreadInfo() {
    uint64_t tid = PlatformThread::GetCurrentThreadId();

    auto *entry = cpuThreadInfoMap.Get(tid);
    if (entry) {
        return entry->second;
    }

    PlatformMutex::Lock(mapAddMutex);

    CpuThreadInfo cpuThreadInfo;
    cpuThreadInfo.threadId = tid;
    cpuThreadInfo.frameMarkerIndexes[currentFrameDataIndex] = cpuThreadInfo.writeMarkerIndex;

    entry = cpuThreadInfoMap.Set(tid, cpuThreadInfo);

    PlatformMutex::Unlock(mapAddMutex);

    return entry->second;
}

void Profiler::PushCpuMarker(int tagIndex) {
    if (IsFrozen()) {
        return;
    }

    CpuThreadInfo &ti = GetCpuThreadInfo();
    CpuMarker &marker = ti.markers[ti.writeMarkerIndex];

    ti.markerIndexStack.Push(ti.writeMarkerIndex);

    ti.writeMarkerIndex = (ti.writeMarkerIndex + 1) % COUNT_OF(ti.markers);

    marker.tagIndex = tagIndex;
    marker.startTime = PlatformTime::Nanoseconds();
    marker.endTime = InvalidTime;
    marker.frameCount = frameCount;
    marker.depth = ti.markerIndexStack.Count();
}

void Profiler::PopCpuMarker() {
    if (IsFrozen()) {
        return;
    }

    CpuThreadInfo &ti = GetCpuThreadInfo();
    assert(!ti.markerIndexStack.IsEmpty());

    int markerIndex = ti.markerIndexStack.Pop();

    CpuMarker &marker = ti.markers[markerIndex];

    marker.endTime = PlatformTime::Nanoseconds();
}

void Profiler::PushGpuMarker(int tagIndex) {
    if (IsFrozen()) {
        return;
    }

    GpuThreadInfo &ti = gpuThreadInfo;
    GpuMarker &marker = ti.markers[ti.writeMarkerIndex];

    ti.markerIndexStack.Push(ti.writeMarkerIndex);

    ti.writeMarkerIndex = (ti.writeMarkerIndex + 1) % COUNT_OF(ti.markers);

    marker.tagIndex = tagIndex;
    rhi.QueryTimestamp(marker.startQueryHandle);
    marker.frameCount = frameCount;
    marker.depth = ti.markerIndexStack.Count();
}

void Profiler::PopGpuMarker() {
    if (IsFrozen()) {
        return;
    }

    GpuThreadInfo &ti = gpuThreadInfo;
    assert(!ti.markerIndexStack.IsEmpty());

    int markerIndex = ti.markerIndexStack.Pop();

    GpuMarker &marker = ti.markers[markerIndex];

    rhi.QueryTimestamp(marker.endQueryHandle);
}

BE_NAMESPACE_END
