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
#ifdef USE_RENDER_TASK
    numThreads = renderer.taskManager.NumThreads();
#else
    numThreads = 1;
#endif

    for (int threadIndex = 0; threadIndex < numThreads; ++threadIndex) {
        DataPerThread* data = &threadData[threadIndex];

        // 커맨드 리스트 풀을 생성한다.
        data->commandListPool = new D3D12CommandListPool(D3D12_COMMAND_LIST_TYPE_DIRECT, 8);

        // 쉐이더에서 사용할 디스크립터 힙을 생성한다.
        data->rootDescriptorPool = new D3D12RootDescriptorPool(16384);

        // 상수 버퍼 디스크립터 풀을 생성한다.
        data->cbvDescriptorPool = new D3D12DescriptorPool(D3D12DescriptorPool::Type::SRV, 8192, false);

        // 다이나믹 상수 버퍼 생성
        data->constantBuffer = D3D12ConstantBuffer::CreateConstantBuffer(65536 * 16);

        // 상수 버퍼를 프로그램이 끝날 때 까지 Map 해놓고 쓴다. (Pinned) 
        data->constantBuffer->buffer->GetResource()->Map(0, nullptr, reinterpret_cast<void **>(&data->mappedConstantBase));

        data->cbvDescriptorHandles.SetGranularity(2048);
        data->cbvDescriptorHandles.Reserve(4096);
    }
}

void D3D12FrameData::Shutdown() {
    for (int threadIndex = 0; threadIndex < numThreads; ++threadIndex) {
        DataPerThread *data = &threadData[threadIndex];

        for (int i = 0; i < data->cbvDescriptorHandles.Count(); ++i) {
            data->cbvDescriptorPool->Free(data->cbvDescriptorHandles[i]);
        }
        data->cbvDescriptorHandles.SetCount(0, false);

        SAFE_DELETE(data->constantBuffer);
        SAFE_DELETE(data->cbvDescriptorPool);
        SAFE_DELETE(data->rootDescriptorPool);
        SAFE_DELETE(data->commandListPool);
    }
}

void D3D12FrameData::BeginFrame() {
    // 쓰레드 별로 사용할 자원을 Reset 한다.
    for (int threadIndex = 0; threadIndex < numThreads; ++threadIndex) {
        DataPerThread *data = &threadData[threadIndex];

        // 이번에 프레임에 사용할 상수 버퍼 디스크립터들을 초기화
        for (int i = 0; i < data->cbvDescriptorHandles.Count(); ++i) {
            data->cbvDescriptorPool->Free(data->cbvDescriptorHandles[i]);
        }
        data->cbvDescriptorHandles.SetCount(0, false);

        // 루트 디스크립터 풀을 비운다.
        data->rootDescriptorPool->Reset();

        // 커맨드 리스트 풀을 비운다.
        data->commandListPool->Clear();

        data->usedConstantBytes = 0;
    }

    // 이번 프레임에 사용할 프레임 데이터를 사용하기 위해서는, GPU 에서 이전 프레임에 대한 렌더링이 완료되야 한다.
    renderer.WaitFence(lastFrameFenceValue);
}

void D3D12FrameData::EndFrame() {
    lastFrameFenceValue = renderer.SignalFence();
}

void *D3D12FrameData::AllocConstant(int threadIndex, int size, D3D12_CPU_DESCRIPTOR_HANDLE* outDescriptorHandlePtr) {
    UINT alignedSize = (UINT)AlignUp(size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    if (alignedSize > D3D12_REQ_CONSTANT_BUFFER_ELEMENT_COUNT * 16) {
        BE_WARNLOG("Constant buffer view size cannot exceeds 64KB limit\n");
        return nullptr;
    }

    DataPerThread* data = &threadData[threadIndex];
    ID3D12Resource *resource = data->constantBuffer->buffer->GetResource();
    UINT maxSize = data->constantBuffer->buffer->GetSize();

    if (data->usedConstantBytes + alignedSize > maxSize) {
        BE_WARNLOG("Out of constant buffer cache\n");
        return nullptr;
    }

    // 상수 버퍼 리소스 (업로드 버퍼) 를 쪼개서 CBV 를 만들어 사용한다.
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {0};
    cbvDesc.BufferLocation = resource->GetGPUVirtualAddress() + data->usedConstantBytes;
    cbvDesc.SizeInBytes = alignedSize;

    D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle = data->cbvDescriptorPool->Alloc();
    if (descriptorHandle.ptr == 0) {
        return nullptr;
    }

    renderer.device->CreateConstantBufferView(&cbvDesc, descriptorHandle);

    data->cbvDescriptorHandles.Append(descriptorHandle);
    *outDescriptorHandlePtr = descriptorHandle;

    void *outPtr = (byte *)data->mappedConstantBase + data->usedConstantBytes;
    data->usedConstantBytes += alignedSize;

    return outPtr;
}
