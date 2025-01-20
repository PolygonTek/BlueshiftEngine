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

#include "RHI.h"

class RenderContext;
class VisCamera;
class VisObject;

enum class RenderCommandId : uint32_t {
    End,
    BeginContext,
    DrawCamera,
    ScreenShot,
    SwapBuffers
};

struct RenderCommandBuffer {
    static constexpr uint32_t       BufferSize = 1024;

    byte                            buffer[BufferSize];
    uint32_t                        used = 0;
};

struct BeginContextRenderCommand {
    RenderCommandId                 commandId;
    RenderContext *                 renderContext;
};

struct DrawCameraRenderCommand {
    RenderCommandId                 commandId;
    const VisCamera *               visCamera;
};

struct ScreenShotRenderCommand {
    RenderCommandId                 commandId;
    int                             x;
    int                             y;
    int                             width;
    int                             height;
    char                            filename[BE1::MaxAbsolutePath];
};

struct SwapBuffersRenderCommand {
    RenderCommandId                 commandId;
};

struct EndRenderCommand {
    RenderCommandId                 commandId;
};

class RenderFrameData {
public:
    void                            Init();
    void                            Shutdown();

                                    // 임시로 할당하는 메모리 (not thread-safe)
    void *                          MemAlloc(int size);
    void *                          ClearedMemAlloc(int size);

    void                            BeginFrameMemAllocs();
    void                            EndFrameMemAllocs();

    uint32_t                        NumVisCameras() const { return numVisCameras; }
    VisCamera *                     GetVisCameras() const { return visCameras; }
    VisCamera *                     AllocVisCamera();

    uint32_t                        NumVisObjects() const { return numVisObjects; }
    VisObject *                     GetVisObjects() const { return visObjects; }
    VisObject *                     AllocVisObject();

    RenderCommandBuffer *           GetCommands() { return &commands; }

    void                            BeginCommands(RenderContext *context);
    void                            CmdDrawCamera(const VisCamera *camera);
    void                            CmdSwapBuffers();
    void                            CmdScreenshot(int x, int y, int width, int height, const char *filename);

                                    // 스레드 별 프레임 데이터 얻기
    RHI::FrameThreadData *          GetThreadData(int threadIndex) { assert(threadIndex >= 0 && threadIndex < COUNT_OF(threadData)); return threadData[threadIndex]; }

    void                            BeginFrame();
    void                            EndFrame();

    uint64_t                        GetFenceValue() const { return fenceValue; }
    void                            SetFenceValue(uint64_t fenceValue) { this->fenceValue = fenceValue; }

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

    void                            InitVisCameras(uint32_t maxVisCameras);
    void                            FreeVisCameras();

    void                            InitVisObjects(int maxVisObjects);
    void                            FreeVisObjects();

    void *                          GetCommandBuffer(uint32_t bytes);

    MemBlock *                      headBlock = nullptr;
    MemBlock *                      currentBlock = nullptr;

    uint32_t                        maxVisCameras = 0;
    uint32_t                        numVisCameras = 0;
    VisCamera *                     visCameras = nullptr;
    uint32_t                        maxVisObjects = 0;
    uint32_t                        numVisObjects = 0;
    VisObject *                     visObjects = nullptr;

    RenderCommandBuffer             commands;

#ifdef USE_TASK_MANAGER
    // 렌더링 태스크에서 사용할 수 있는 최대 스레드 개수
    static constexpr uint32_t       MaxRenderTaskThreads = 16;

    RHI::FrameThreadData *          threadData[MaxRenderTaskThreads] = {};
#else
    RHI::FrameThreadData *          threadData[1] = {};
#endif
    int                             numRenderTaskThreads = 0;

    uint64_t                        fenceValue = 0;
};

BE_INLINE void RenderFrameData::BeginFrame() {
    // 쓰레드 별로 사용할 자원을 Reset 한다.
    for (int threadIndex = 0; threadIndex < numRenderTaskThreads; ++threadIndex) {
        threadData[threadIndex]->Reset();
    }

    // 이번 프레임에 사용할 프레임 데이터를 사용하기 위해서는, GPU 에서 이전 프레임에 대한 렌더링이 완료되야 한다.
    RHI::renderer->WaitFence(fenceValue);
}

BE_INLINE void RenderFrameData::EndFrame() {
    fenceValue = RHI::renderer->SignalFence(RHI::CommandQueueType::Graphics);
}

BE_INLINE void RenderFrameData::BeginCommands(RenderContext *context) {
    BeginContextRenderCommand *cmd = (BeginContextRenderCommand *)GetCommandBuffer(sizeof(BeginContextRenderCommand));
    if (!cmd) {
        return;
    }

    cmd->commandId = RenderCommandId::BeginContext;
    cmd->renderContext = context;
}

BE_INLINE void RenderFrameData::CmdDrawCamera(const VisCamera *camera) {
    DrawCameraRenderCommand *cmd = (DrawCameraRenderCommand *)GetCommandBuffer(sizeof(DrawCameraRenderCommand));
    if (!cmd) {
        return;
    }

    cmd->commandId = RenderCommandId::DrawCamera;
    cmd->visCamera = camera;
}

BE_INLINE void RenderFrameData::CmdSwapBuffers() {
    SwapBuffersRenderCommand *cmd = (SwapBuffersRenderCommand *)GetCommandBuffer(sizeof(SwapBuffersRenderCommand));
    if (!cmd) {
        return;
    }

    cmd->commandId = RenderCommandId::SwapBuffers;
}

BE_INLINE void RenderFrameData::CmdScreenshot(int x, int y, int width, int height, const char *filename) {
    ScreenShotRenderCommand *cmd = (ScreenShotRenderCommand *)GetCommandBuffer(sizeof(ScreenShotRenderCommand));
    if (!cmd) {
        return;
    }

    cmd->commandId = RenderCommandId::ScreenShot;
    cmd->x = x;
    cmd->y = y;
    cmd->width = width;
    cmd->height = height;
    BE1::Str::Copynz(cmd->filename, filename, COUNT_OF(cmd->filename));
}
