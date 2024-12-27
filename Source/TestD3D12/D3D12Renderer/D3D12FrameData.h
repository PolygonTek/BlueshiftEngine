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

#include "D3D12Common.h"
#include "D3D12ConstantBuffer.h"
#include "D3D12VertexBuffer.h"
#include "D3D12IndexBuffer.h"

class D3D12Renderer;
class D3D12CommandListPool;
class D3D12RootDescriptorPool;
class D3D12DescriptorPool;
class D3D12Buffer;
class D3D12VisObject;

class D3D12DynamicAllocation {
public:
    D3D12DynamicAllocation(uint64_t size);
    ~D3D12DynamicAllocation();

    D3D12Buffer *                   buffer = nullptr;
    void *                          mappedBase = nullptr;
    UINT                            usedBytes = 0;
};

class D3D12FrameData {
    friend class D3D12Renderer;

public:
    void                            Init();
    void                            Shutdown();

                                    // 프레임 별로 임시로 할당하는 메모리 (not thread-safe)
    void *                          MemAlloc(int size);
    void *                          ClearedMemAlloc(int size);
    D3D12VisObject *                AllocVisObjects(int numVisObjects);
    void                            FreeVisObjects();

    int                             NumVisObjects() const { return numVisObjects; }
    D3D12VisObject *                GetVisObjects() { return visObjects; }

    void                            ClearMemAllocs();

    RHIRenderer::ConstantBuffer *   AllocConstant(int threadIndex, uint32_t size);
    RHIRenderer::VertexBuffer *     AllocVertex(int threadIndex, uint32_t vertexSize, uint32_t count);
    RHIRenderer::IndexBuffer *      AllocIndex(int threadIndex, uint32_t indexSize, uint32_t count);
    RHIRenderer::Buffer *           AllocBuffer(int threadIndex, bool shaderStorage, Image::Format::Enum format, uint32_t structureByteStride, uint32_t count);

    void                            BeginFrame();
    void                            EndFrame();

    UINT64                          GetFenceValue() const { return fenceValue; }
    void                            SetFenceValue(UINT64 fenceValue) { this->fenceValue = fenceValue; }

private:
    struct MemBlock {
        MemBlock *                  next;
        int32_t                     size;
        int32_t                     used;
        byte *                      base;
    };

    void                            InitMemBlocks();
    void                            ClearMemBlocks();
    MemBlock *                      AllocMemBlock();

    MemBlock *                      headBlock;
    MemBlock *                      currentBlock;

    int                             numVisObjects = 0;
    D3D12VisObject *                visObjects = nullptr;

    struct DataPerThread {
        static constexpr uint32_t   MaxRootParameters = 64;

        D3D12CommandListPool *      graphicsCommandListPool = nullptr;
        D3D12CommandListPool *      computeCommandListPool = nullptr;
        Array<D3D12DynamicAllocation *> dynamicAllocations;
        Array<D3D12ConstantBuffer>  dynamicConstantBuffers;
        Array<D3D12VertexBuffer>    dynamicVertexBuffers;
        Array<D3D12IndexBuffer>     dynamicIndexBuffers;
        Array<D3D12Buffer>          dynamicBuffers;
        D3D12DescriptorPool *       dynamicDescriptorPool = nullptr;
        Array<D3D12_CPU_DESCRIPTOR_HANDLE> dynamicDescriptorHandles;
        D3D12RootDescriptorPool *   rootDescriptorPool = nullptr;
        D3D12_CPU_DESCRIPTOR_HANDLE psoDescriptorHandles[MaxRootParameters][64] = { CD3DX12_CPU_DESCRIPTOR_HANDLE() };
        const RHIRenderer::GPUResource *cbvResources[256] = {};
        const RHIRenderer::GPUResource *srvResources[256] = {};
        const RHIRenderer::GPUResource *uavResources[256] = {};
        uint32_t                    rootConstants[64] = {};
    };

#ifdef USE_RENDER_TASK
    DataPerThread                   threadData[MaxRenderTaskThreads];
#else
    DataPerThread                   threadData[1];
#endif
    int                             numThreads = 0;

    UINT64                          fenceValue = 0;
};
