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

#include "../RHI.h"
#include "D3D12Common.h"

#ifdef USE_D3D12_MEMALLOC
#include "D3D12MemoryAllocator/D3D12MemAlloc.h"
#endif

class D3D12FrameThreadData;

/*
-------------------------------------------------------------------------------
    버퍼 리소스의 정렬에 대한 규칙

    1. 리소스 힙에 배치 단계 (Placed Resource or Committed Resource) 에서의 오프셋 정렬.
       대부분 D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT (64KB) 로 정렬해야 한다.
       만약 리소스 크기 <= 64KB 이고, UAV/RTV/DSV 가 아니라면 D3D12_SMALL_RESOURCE_PLACEMENT_ALIGNMENT (4KB) 로 정렬 가능하다.

    2. 뷰 생성 단계에서의 오프셋/사이즈 정렬
       * CBV 는 오프셋/사이즈 D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT (256B) 정렬을 요구한다.
       * Byte Address Buffer SRV/UAV 는 4B 정렬을 요구한다.
       * Structured Buffer SRV/UAV 는 구조체의 크기가 4의 배수여야 한다. (최대 2048B)
       * VBV/IBV 는 특별한 정렬을 요구하지 않는다.

    3. cbuffer/struct 내부의 변수 pack 정렬
       내부의 변수는 16B 정렬된다.
-------------------------------------------------------------------------------
*/

class D3D12Buffer : public RHI::Buffer {
    friend class D3D12Renderer;
    friend class D3D12FrameThreadData;

public:
    D3D12Buffer() = default;
    virtual ~D3D12Buffer() { Release(); }

    void                            Release();

    virtual void *                  GetNativeResource() const override { return GetResource(); }
    virtual const void *            GetNativeBufferObject() const override { return this; }
    virtual bool                    IsValidSubresource(RHI::SubresourceType type, int subresourceIndex) const override;

    ID3D12Resource *                GetResource() const;
    uint64_t                        GetSize();

private:
#ifdef USE_D3D12_MEMALLOC
    D3D12MA::Allocation *           bufferAllocation = nullptr;
#else
    ID3D12Resource *                bufferResource = nullptr;
#endif
    uint64_t                        size = 0;
    uint32_t                        structureByteStride = 0;
    BE1::Image::Format              format = BE1::Image::Format::Unknown;
    D3D12SRVDescriptor              srvDescriptor;
    D3D12UAVDescriptor              uavDescriptor;
    BE1::Array<D3D12SRVDescriptor>  subresourceSrvDescriptors = (16);
    BE1::Array<D3D12UAVDescriptor>  subresourceUavDescriptors = (16);
};

BE_INLINE ID3D12Resource *D3D12Buffer::GetResource() const {
#ifdef USE_D3D12_MEMALLOC
    return bufferAllocation->GetResource();
#else
    return bufferResource;
#endif
}

BE_INLINE uint64_t D3D12Buffer::GetSize() {
#ifdef USE_D3D12_MEMALLOC
    return bufferAllocation->GetSize();
#else
    return size;
#endif
}
