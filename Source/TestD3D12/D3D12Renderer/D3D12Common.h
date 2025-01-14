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

#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3d11on12.h>
#include <d3d12video.h>
#include <d3dx12.h>
#include <d3dcompiler.h>
#include <dxcapi.h>
#include <dxgidebug.h>
#include <DirectXMath.h>
#include <wrl/client.h> // ComPtr

#if defined(_DEBUG) || defined(_DEVELOPMENT)
#define PROFILE_BUILD
#include <pix3.h>
#endif

// D3D12 Memory Allocator 사용 여부
#define USE_D3D12_MEMALLOC

// 스테이트 캐싱 사용 여부
#define USE_STATE_CACHE_FOR_COMMAND_LIST

template <typename T>
T *NormalizePointer(T *&ptr) noexcept {
    return ptr;
}

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
