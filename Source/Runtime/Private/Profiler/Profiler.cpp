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
    frameCount = 0;

    for (int i = 0; i < COUNT_OF(gpuThreadInfo.markers); i++) {
        auto &marker = gpuThreadInfo.markers[i];

        marker.startQueryHandle = rhi.CreateQuery(RHI::TimestampQuery);
        marker.endQueryHandle = rhi.CreateQuery(RHI::TimestampQuery);
    }

    mapAddMutex = PlatformMutex::Create();
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
    frameCount++;
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
    cpuThreadInfo.writeMarkerIndex = 0;

    entry = cpuThreadInfoMap.Set(tid, cpuThreadInfo);

    PlatformMutex::Unlock(mapAddMutex);

    return entry->second;
}

void Profiler::PushCpuMarker(const char *name, const Color3 &color) {
    CpuThreadInfo &ti = GetCpuThreadInfo();
    CpuMarker &marker = ti.markers[ti.writeMarkerIndex];

    ti.markerIndexStack.Push(ti.writeMarkerIndex);

    ti.writeMarkerIndex = (ti.writeMarkerIndex + 1) % COUNT_OF(ti.markers);

    Str::Copynz(marker.name, name, COUNT_OF(marker.name));
    marker.color = color;
    marker.startTime = PlatformTime::Nanoseconds();
    marker.endTime = InvalidTime;
    marker.frameCount = frameCount;
    marker.depth = ti.markerIndexStack.Count();
}

void Profiler::PopCpuMarker() {
    CpuThreadInfo &ti = GetCpuThreadInfo();
    assert(!ti.markerIndexStack.IsEmpty());

    int markerIndex = ti.markerIndexStack.Pop();

    CpuMarker &marker = ti.markers[markerIndex];

    marker.endTime = PlatformTime::Nanoseconds();
}

void Profiler::PushGpuMarker(const char *name, const Color3 &color) {
    GpuThreadInfo &ti = gpuThreadInfo;
    GpuMarker &marker = ti.markers[ti.writeMarkerIndex];

    ti.markerIndexStack.Push(ti.writeMarkerIndex);

    ti.writeMarkerIndex = (ti.writeMarkerIndex + 1) % COUNT_OF(ti.markers);

    Str::Copynz(marker.name, name, COUNT_OF(marker.name));
    marker.color = color;
    rhi.QueryTimestamp(marker.startQueryHandle);
    marker.frameCount = frameCount;
    marker.depth = ti.markerIndexStack.Count();
}

void Profiler::PopGpuMarker() {
    GpuThreadInfo &ti = gpuThreadInfo;
    assert(!ti.markerIndexStack.IsEmpty());

    int markerIndex = ti.markerIndexStack.Pop();

    GpuMarker &marker = ti.markers[markerIndex];

    rhi.QueryTimestamp(marker.endQueryHandle);
}

BE_NAMESPACE_END
