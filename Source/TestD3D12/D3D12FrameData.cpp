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
#include "D3D12FrameData.h"
#include "D3D12Renderer.h"
#include "D3D12CommandListPool.h"
#include "D3D12RootDescriptorPool.h"
#include "D3D12DescriptorPool.h"
#include "D3D12ConstantBuffer.h"
#include "D3D12Buffer.h"

void D3D12FrameData::Init() {
    commandListPool = new D3D12CommandListPool;
    commandListPool->Init(D3D12_COMMAND_LIST_TYPE_DIRECT, 16);

    // 쉐이더에서 사용할 디스크립터 힙을 생성한다.
    rootDescriptorPool = new D3D12RootDescriptorPool;
    rootDescriptorPool->Init(65536);

    // 다이나믹 상수 버퍼 생성
    constantBuffer = D3D12ConstantBuffer::CreateConstantBuffer(65536 * 64);

    // 상수 버퍼를 프로그램이 끝날 때 까지 Map 해놓고 쓴다. (Pinned) 
    constantBuffer->buffer->GetResource()->Map(0, nullptr, reinterpret_cast<void **>(&mappedConstantBase));

    cbvDescriptorHandles.SetGranularity(64);
}

void D3D12FrameData::Shutdown() {
    for (int i = 0; i < cbvDescriptorHandles.Count(); ++i) {
        renderer.srvDescriptorPool->Free(cbvDescriptorHandles[i]);
    }
    cbvDescriptorHandles.SetCount(0, false);

    SAFE_DELETE(constantBuffer);
    SAFE_DELETE(commandListPool);
    SAFE_DELETE(rootDescriptorPool);
}

void *D3D12FrameData::AllocConstant(int size, D3D12_CPU_DESCRIPTOR_HANDLE* outDescriptorHandlePtr) {
    UINT alignedSize = (UINT)AlignUp(size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    if (alignedSize > D3D12_REQ_CONSTANT_BUFFER_ELEMENT_COUNT * 16) {
        BE_WARNLOG("Constant buffer view size cannot exceeds 64KB limit\n");
        return nullptr;
    }

    ID3D12Resource *resource = constantBuffer->buffer->GetResource();
    UINT maxSize = constantBuffer->buffer->GetSize();

    if (usedConstantBytes + alignedSize > maxSize) {
        BE_WARNLOG("Out of constant buffer cache\n");
        return nullptr;
    }

    // 상수 버퍼 리소스 (업로드 버퍼) 를 쪼개서 CBV 를 만들어 사용한다.
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {0};
    cbvDesc.BufferLocation = resource->GetGPUVirtualAddress() + usedConstantBytes;
    cbvDesc.SizeInBytes = alignedSize;

    D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle = renderer.srvDescriptorPool->Alloc();
    if (descriptorHandle.ptr == 0) {
        return nullptr;
    }

    renderer.device->CreateConstantBufferView(&cbvDesc, descriptorHandle);

    cbvDescriptorHandles.Append(descriptorHandle);
    *outDescriptorHandlePtr = descriptorHandle;

    void *outPtr = (byte *)mappedConstantBase + usedConstantBytes;
    usedConstantBytes += alignedSize;

    return outPtr;
}

void D3D12FrameData::BeginRender() {
    for (int i = 0; i < cbvDescriptorHandles.Count(); ++i) {
        renderer.srvDescriptorPool->Free(cbvDescriptorHandles[i]);
    }
    cbvDescriptorHandles.SetCount(0, false);

    usedConstantBytes = 0;
}
