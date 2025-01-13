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

template <typename T>
T *NormalizePointer(T *&ptr) noexcept {
    return ptr;
}

#if defined(USE_PIX_MARKERS) && defined(USE_PIX)
#define PIX_CPU_BEGIN_EVENT(colorIndex, string) PIXBeginEvent(PIX_COLOR_INDEX(colorIndex), string)
#define PIX_CPU_END_EVENT() PIXEndEvent()
#define PIX_CPU_SCOPED_EVENT(colorIndex, string) PIXScopedEvent(PIX_COLOR_INDEX(colorIndex), string)
#define PIX_CPU_MARKER(colorIndex, string) PIXSetMarker(PIX_COLOR_INDEX(colorIndex), string)
#define PIX_BEGIN_EVENT(context, colorIndex, string) PIXBeginEvent(NormalizePointer(context), PIX_COLOR_INDEX(colorIndex), string)
#define PIX_END_EVENT(context) PIXEndEvent(NormalizePointer(context))
#define PIX_SCOPED_EVENT(context, colorIndex, string) PIXScopedEvent(NormalizePointer(context), PIX_COLOR_INDEX(colorIndex), string)
#define PIX_MARKER(context, colorIndex, string) PIXSetMarker(NormalizePointer(context), PIX_COLOR_INDEX(colorIndex), string)
#else
#define PIX_CPU_BEGIN_EVENT(colorIndex, string)
#define PIX_CPU_END_EVENT()
#define PIX_CPU_SCOPED_EVENT(colorIndex, string)
#define PIX_CPU_MARKER(colorIndex, string)
#define PIX_BEGIN_EVENT(context, colorIndex, string)
#define PIX_END_EVENT(context)
#define PIX_SCOPED_EVENT(context, colorIndex, string)
#define PIX_MARKER(context, colorIndex, string)
#endif

struct D3D12SRVDescriptor {
    D3D12_SHADER_RESOURCE_VIEW_DESC     srvDesc = {};
    D3D12_CPU_DESCRIPTOR_HANDLE         cpuDescriptorHandle = {};
};

struct D3D12RTVDescriptor {
    D3D12_RENDER_TARGET_VIEW_DESC       rtvDesc = {};
    D3D12_CPU_DESCRIPTOR_HANDLE         cpuDescriptorHandle = {};
};

struct D3D12DSVDescriptor {
    D3D12_DEPTH_STENCIL_VIEW_DESC       dsvDesc = {};
    D3D12_CPU_DESCRIPTOR_HANDLE         cpuDescriptorHandle = {};
};

struct D3D12UAVDescriptor {
    D3D12_UNORDERED_ACCESS_VIEW_DESC    uavDesc = {};
    D3D12_CPU_DESCRIPTOR_HANDLE         cpuDescriptorHandle = {};
};
