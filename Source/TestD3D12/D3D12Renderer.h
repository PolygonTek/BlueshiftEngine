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

#ifdef USE_D3D12_MEMALLOC
#include "D3D12MemoryAllocator/D3D12MemAlloc.h"
#endif

#include "D3D12FrameData.h"
#include "D3D12RenderObject.h"

class D3D12CommandList;
class D3D12DescriptorPool;

#ifdef USE_RENDER_THREAD
enum class FrameSyncState : byte {
    WaitingForUpdateCompleted,  // (렌더 스레드가 렌더링이 완료되어) 메인 스레드의 다음 업데이트 작업이 완료되기를 기다리는 상태
    WaitingForRenderCompleted   // (메인 스레드가 업데이트가 완료되어) 렌더 스레드의 다음 렌더링 작업이 완료되기를 기다리는 상태
};
#endif

struct D3D12PendingResource {
    UINT64                              fenceValue = 0;
    ID3D12Resource*                     resource = nullptr;
};

class D3D12VisObject {
    ALIGN_AS32 Mat4                     modelViewProjMatrix;
    ALIGN_AS32 Mat3x4                   modelViewMatrix;
};

class D3D12Renderer {
public:
    void                                Init(HWND hwnd);
    void                                Shutdown();

    bool                                IsInitialized() const { return initialized; }

    void                                BeginFrame();
    void                                EndFrame();
    void                                SwapChainBuffers();

    void                                OnResize(int width, int height);

    void                                CreateRTVs();
    void                                CreateDSV(int width, int height);

    D3D12CommandList *                  FlushCommandList(D3D12CommandList *commandList);

    UINT64                              SignalFence();
    bool                                IsFenceComplete(UINT64 checkFenceValue);
    void                                WaitFence(UINT64 expectedFenceValue);
    void                                Finish();

    void                                WaitAllFrameFences();

    void                                MarkForRelease(ID3D12Resource *resource);
    void                                FreePendingResources(bool waitPendings = false);

    void                                PrintCompileErrorMessages(ID3DBlob *errorBlob);

#ifdef USE_D3D12_MEMALLOC
    void                                PrintMemoryAllocatorStats();
#endif

    struct RenderObjectTaskDesc {
        int                             threadIndex = -1;
        int                             renderObjectStartIndex = -1;
        int                             renderObjectEndIndex = -1;
        D3D12CommandList *              activeCommandList = nullptr;
    };

    int                                 AddRenderObject(const D3D12RenderObject::State &def);
    void                                UpdateRenderObject(int handle, const D3D12RenderObject::State &def);
    void                                RemoveRenderObject(int handle);

    void                                RenderScene();

    void                                RenderFrame();
    void                                DrawRenderObjects(int threadIndex, D3D12CommandList *commandList, int startIndex, int endIndex);
    void                                DrawRenderObjectsWithoutTask();
#ifdef USE_RENDER_TASK
    void                                DrawRenderObjectsWithTask(int numTasks);
    void                                DrawRenderObjectsByTask(D3D12Renderer::RenderObjectTaskDesc *taskDesc);
#endif

    static constexpr UINT               NumSwapChainBuffers = 3;

    ID3D12Device5 *                     device = nullptr;
    DXGI_ADAPTER_DESC1                  adapterDesc = {};
    IDXGISwapChain3 *                   swapChain = nullptr;
    ID3D12CommandQueue *                commandQueue = nullptr;
    D3D12CommandListPool *              commandListPool = nullptr;
    D3D12CommandList *                  resourceCommandList = nullptr;
    ID3D12Fence *                       fence = nullptr;
    UINT64                              fenceValue = 0;
    HANDLE                              fenceEventHandle = nullptr;

#ifdef USE_D3D12_MEMALLOC
    D3D12MA::Allocator *                allocator = nullptr;
#endif
    UINT                                descriptorHandleSize[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
    ID3D12DescriptorHeap *              rtvDescriptorHeap = nullptr;
    ID3D12DescriptorHeap *              dsvDescriptorHeap = nullptr;
    ID3D12Resource *                    renderTargetBuffers[NumSwapChainBuffers] = {};
    ID3D12Resource *                    depthStencilBuffer = nullptr;

    D3D12_CPU_DESCRIPTOR_HANDLE         rtvDescriptorHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE();
    D3D12_CPU_DESCRIPTOR_HANDLE         dsvDescriptorHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE();

    UINT                                currentBackBufferIndex = 0;
    D3D12_VIEWPORT                      viewport = {};
    D3D12_RECT                          scissorRect = {};
    D3D12DescriptorPool *               srvDescriptorPool = nullptr;
    D3D12DescriptorPool *               rtvDescriptorPool = nullptr;
    D3D12DescriptorPool *               dsvDescriptorPool = nullptr;

    UINT                                frameCount = 0;
    D3D12FrameData                      frameData[NumFrameResources];
    D3D12FrameData *                    currentFrameData = nullptr;
    UINT                                currentFrameIndex = 0;

    D3D12PendingResource *              pendingResourceBuffer = nullptr;
    int                                 maxPendingResources = 0;
    int                                 headPendingIndex = 0;
    int                                 tailPendingIndex = 0;

    Array<D3D12RenderObject *>          renderObjects;
    Array<D3D12RenderObject *>          visObjects[2];
    Array<RenderObjectTaskDesc>         renderObjectTaskDescs;

#ifdef USE_RENDER_TASK
    TaskManager                         taskManager = TaskManager(MaxRenderTasks);
#endif

#ifdef USE_RENDER_THREAD
    friend unsigned int                 RenderThreadProc(void *param);

    void                                InitRenderThread();
    void                                ShutdownRenderThread();
    void                                WaitRenderCompleted();

    PlatformSRWLock *                   smpLock = nullptr;
    PlatformCondition *                 renderCompletedCondition = nullptr;
    PlatformCondition *                 updateCompletedCondition = nullptr;
    PlatformThread *                    renderThread = nullptr;
    bool                                isStoppingRenderThread = false;
    int                                 renderFrameIndex = 1;
    FrameSyncState                      frameSyncState = FrameSyncState::WaitingForUpdateCompleted;
#endif

    bool                                initialized = false;
};

extern D3D12Renderer                    renderer;
