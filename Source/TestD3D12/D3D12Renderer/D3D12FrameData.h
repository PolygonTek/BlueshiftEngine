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

class D3D12Renderer;
class D3D12CommandListPool;
class D3D12RootDescriptorPool;
class D3D12DescriptorPool;
class D3D12ConstantBuffer;
class D3D12VisObject;

class D3D12GPUSubResource : public RHIRenderer::GPUSubResource {
public:
    D3D12_CPU_DESCRIPTOR_HANDLE     descriptorHandle = {0};
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

    RHIRenderer::GPUSubResource *   AllocConstant(int threadIndex, int size);

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
        D3D12CommandListPool *      graphicsCommandListPool = nullptr;
        D3D12CommandListPool *      computeCommandListPool = nullptr;
        D3D12RootDescriptorPool *   rootDescriptorPool = nullptr;
        D3D12DescriptorPool *       cbvDescriptorPool = nullptr;
        D3D12ConstantBuffer *       constantBuffer = nullptr;
        void *                      mappedConstantBase = nullptr;
        UINT                        usedConstantBytes = 0;
        Array<D3D12_CPU_DESCRIPTOR_HANDLE> cbvDescriptorHandles;
        D3D12_CPU_DESCRIPTOR_HANDLE psoDescriptorHandles[64] = { CD3DX12_CPU_DESCRIPTOR_HANDLE() };
        const RHIRenderer::GPUResource *cbvResources[64] = {};
        const RHIRenderer::GPUResource *srvResources[64] = {};
        const RHIRenderer::GPUResource *uavResources[64] = {};
        uint32_t                    rootConstants[64] = {};
        Array<D3D12GPUSubResource>  subResources;
    };

#ifdef USE_RENDER_TASK
    DataPerThread                   threadData[MaxRenderTaskThreads];
#else
    DataPerThread                   threadData[1];
#endif
    int                             numThreads = 0;

    UINT64                          fenceValue = 0;
};
