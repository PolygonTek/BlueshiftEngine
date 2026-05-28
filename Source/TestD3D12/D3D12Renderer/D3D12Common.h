// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http://www.apache.org/licenses/LICENSE-2.0
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

// 셰이더 디버깅 기능 사용 여부 (셰이더 컴파일 시 디버깅 정보 포함 여부)
//#define USE_SHADER_DEBUG

// D3D12 셰이더 캐싱 시, 컴파일된 셰이더 (DXIL) 을 파일로 저장하여 다음 실행 시 로드하는 기능 사용 여부
#ifndef USE_SHADER_DEBUG
#define USE_COMPILED_SHADER
#endif

// D3D12 PSO 캐싱 시, PSO 블롭 (직렬화된 PSO 데이터) 을 파일로 저장하여 다음 실행 시 로드하는 기능 사용 여부
#ifndef USE_SHADER_DEBUG
#define USE_CACHED_PSO_BLOB
#endif

// 스테이트 캐싱 사용 여부
#define USE_STATE_CACHE_FOR_COMMAND_LIST

template <typename T>
T *NormalizePointer(T *&ptr) noexcept {
    return ptr;
}

struct D3D12SRVEntry {
    D3D12_SHADER_RESOURCE_VIEW_DESC     viewDesc = {};
    D3D12_CPU_DESCRIPTOR_HANDLE         cpuDescriptorHandle = {};
    uint32_t                            bindlessIndex = 0;
};

struct D3D12RTVEntry {
    D3D12_RENDER_TARGET_VIEW_DESC       viewDesc = {};
    D3D12_CPU_DESCRIPTOR_HANDLE         cpuDescriptorHandle = {};
};

struct D3D12DSVEntry {
    D3D12_DEPTH_STENCIL_VIEW_DESC       viewDesc = {};
    D3D12_CPU_DESCRIPTOR_HANDLE         cpuDescriptorHandle = {};
};

struct D3D12UAVEntry {
    D3D12_UNORDERED_ACCESS_VIEW_DESC    viewDesc = {};
    D3D12_CPU_DESCRIPTOR_HANDLE         cpuDescriptorHandle = {};
    uint32_t                            bindlessIndex = 0;
};
