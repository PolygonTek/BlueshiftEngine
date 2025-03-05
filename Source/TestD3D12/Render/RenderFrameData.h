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

#include "RHI.h"

class RenderContext;
class Texture;
class Font;
class VisCamera;
class VisObject;
enum class DrawTextFlag : uint16_t;

enum class RenderCommandId : uint8_t {
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

/*
-------------------------------------------------------------------------------
    RenderFrameData

    프레임에 필요한 렌더링 데이터들을 관리한다.

    1. 프레임 임시 메모리
    2. 렌더 커맨더 버퍼
    3. 렌더 태스크 스레드 데이터
-------------------------------------------------------------------------------
*/

class RenderFrameData {
public:
    void                            Init();
    void                            Shutdown();

                                    // 이번 프레임의 임시 메모리 할당 초기화 (메인 스레드에서 호출됨)
    void                            BeginFrameMemAllocs();

                                    // 프레임에 필요한 임시 메모리 할당 (not thread-safe), 이번 프레임이 끝나면 자동으로 해제된다.
    void *                          MemAlloc(int size);
    void *                          ClearedMemAlloc(int size);

    uint32_t                        NumVisCameras() const { return numVisCameras; }
    VisCamera *                     GetVisCameras() const { return visCameras; }
    VisCamera *                     AllocVisCamera();

    uint32_t                        NumVisObjects() const { return numVisObjects; }
    VisObject *                     GetVisObjects() const { return visObjects; }
    VisObject *                     AllocVisObject();

                                    // 렌더 커맨드 버퍼 리턴
    RenderCommandBuffer *           GetCommands() { return &commands; }

    void                            CmdBeginContext(RenderContext *context);
    void                            CmdDrawCamera(const VisCamera *camera);
    void                            CmdSwapBuffers();
    void                            CmdScreenshot(int x, int y, int width, int height, const char *filename);
    void                            CmdEnd();

                                    // 프레임의 렌더 태스크 스레드 별 데이터 얻기
    RHI::FrameThreadData *          GetThreadData(int threadIndex) { assert(threadIndex >= 0 && threadIndex < numRenderTaskThreads); return threadData[threadIndex]; }

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

    RHI::FrameThreadData **         threadData = nullptr;
    int                             numRenderTaskThreads = 0;

    uint64_t                        fenceValue = 0;
};

BE_INLINE void RenderFrameData::BeginFrame() {
    // 프론트 엔드에서 이번 프레임의 프레임 데이터를 사용하기 위해서는, 백엔드에서의 사용이 완료되야 한다.
    RHI::renderer->WaitFence(fenceValue);

    // 스레드 별로 사용할 자원을 초기화한다.
    for (int threadIndex = 0; threadIndex < numRenderTaskThreads; ++threadIndex) {
        threadData[threadIndex]->Reset();
    }

    // 프레임 메모리를 초기화한다.
    BeginFrameMemAllocs();
}

BE_INLINE void RenderFrameData::EndFrame() {
    // 커맨드 버퍼에 커맨드의 끝을 기록.
    *(uint32_t *)(commands.buffer + commands.used) = static_cast<uint32_t>(RenderCommandId::End);

    // 커맨드 버퍼 비우기.
    commands.used = 0;
}

BE_INLINE void RenderFrameData::CmdBeginContext(RenderContext *context) {
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

BE_INLINE void RenderFrameData::CmdEnd() {
    EndRenderCommand *cmd = (EndRenderCommand *)GetCommandBuffer(sizeof(EndRenderCommand));
    if (!cmd) {
        return;
    }

    cmd->commandId = RenderCommandId::End;
}
