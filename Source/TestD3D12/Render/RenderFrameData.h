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

#include "Containers/HashIndex.h"
#include "RHI.h"
#include "Shaders/ShaderInterop.h"

class RenderContext;
class Texture;
class Material;
class MeshInstance;
class Font;
struct VisCamera;
struct VisObject;
enum class DrawTextFlag : uint16_t;

enum class FrameSyncState : uint8_t {
    RenderCompleted,        // (렌더 스레드의 렌더링이 완료되어) 메인 스레드의 다음 업데이트 작업이 완료되기를 기다리는 상태
    UpdateCompleted         // (메인 스레드의 업데이트가 완료되어) 렌더 스레드의 다음 렌더링 작업이 완료되기를 기다리는 상태
};

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

struct RenderContextPipelineSnapshot {
    RHI::PipelineState *            imagePSO = nullptr;
};

struct BeginContextRenderCommand {
    RenderCommandId                 commandId;
    RenderContext *                 renderContext;
    RenderContextPipelineSnapshot   pipelines;
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
---------------------------------------------------------------------------------------------------

    RenderFrameData

    한 프레임의 렌더링에 필요한 데이터들을 관리한다.

    1. 한 프레임 임시 시스템 메모리
    2. 렌더 커맨드 버퍼 (한 프레임에 쌓여진 렌더링 명령)
    3. 프레임의 스레드 별 렌더 태스크 데이터 (RHI::FrameThreadData 배열)
        - 렌더링에 필요한 자원/데이터를 스레드 별로 분배한 것
        - 렌더 태스크의 최대 스레드 수만큼 미리 생성된다.
        - 스레드를 사용하지 않는 프론트엔드/백엔드 코드에서는 그냥 0 번째 요소를 사용한다.
        - 스레드간 자원/데이터가 독립적이므로 동기화가 필요없다.

---------------------------------------------------------------------------------------------------
*/

class RenderFrameData {
    friend class RenderSystem;
    friend class RenderContext;

public:
    void                            Init(int32_t frameIndex, int32_t numFrames);
    void                            Shutdown();

                                    // 이번 프레임 동안 사용할 임시 시스템 메모리를 할당한다.
                                    // 할당된 메모리는 다음 프레임에서 재사용될 때 일괄적으로 해제된다.
                                    // 현재는 프론트엔드에서 이번 프레임의 렌더 데이터를 구성하는 동안에만 사용한다.
                                    // 프론트엔드/백엔드 또는 여러 스레드에서 동시에 할당하려면 별도의 동기화가 필요하다.
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

                                    // 렌더 커맨드들..
    void                            CmdBeginContext(RenderContext *context);
    void                            CmdDrawCamera(const VisCamera *camera);
    void                            CmdSwapBuffers();
    void                            CmdScreenshot(int x, int y, int width, int height, const char *filename);
    void                            CmdEnd();

                                    // 렌더 태스크의 스레드 별 데이터 얻기
    RHI::FrameThreadData *          GetThreadData(int32_t threadIndex) { assert(threadIndex >= 0 && threadIndex < numRenderTaskThreads); return threadData[threadIndex]; }

    bool                            IsStatEnabled() const { return statEnabled; }
    void                            SetStatEnabled(bool enabled) { statEnabled = enabled; }

    Material *                      RetainMaterial(const SharedPtr<Material> &material);
    MeshInstance *                  RetainMeshInstance(const SharedPtr<MeshInstance> &meshInstance);
    Font *                          RetainFont(const SharedPtr<Font> &font);

    uint32_t                        GetShaderMaterialTableBindlessIndex() const { return shaderMaterialTableBindlessIndex; }

                                    // 셰이더 머티리얼 테이블에 머티리얼을 등록하고, 테이블 인덱스를 리턴한다.
    uint32_t                        RegisterShaderMaterial(const Material *material);
    bool                            UploadShaderMaterialTable(RHI::CommandList *commandList);

                                    // 프론트 엔드에서 프레임 데이터의 사용을 시작
    void                            BeginFrame();

                                    // 프론트 엔드에서 프레임 데이터의 사용을 종료
    void                            EndFrame();

    void                            SetFrameFenceValue(uint64_t fenceValue) { this->frameFenceValue = fenceValue; }
    void                            WaitFrameFence() const { RHI::renderer->WaitFence(RHI::CommandQueueType::Graphics, frameFenceValue); }

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

                                    // 이번 프레임의 임시 시스템 메모리 할당 초기화 (메인 스레드에서 호출됨)
    void                            BeginFrameMemAllocs();

    void                            InitVisCameras(uint32_t maxVisCameras);
    void                            FreeVisCameras();

    void                            InitVisObjects(int maxVisObjects);
    void                            FreeVisObjects();

    void                            ClearRetainedMaterials();
    void                            ClearRetainedMeshInstances();
    void                            ClearRetainedFonts();

    void                            ClearShaderMaterialTable();
    void                            DestroyShaderMaterialBuffers();
    bool                            EnsureShaderMaterialBufferCapacity(uint32_t shaderMaterialCount);
    void                            BuildShaderMaterial(const Material *material, ShaderMaterial &outShaderMaterial) const;

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

    FrameSyncState                  frameSyncState = FrameSyncState::RenderCompleted;
    uint64_t                        frameFenceValue = 0;
    uint64_t                        frameSerial = 0;
    static uint64_t                 globalFrameSerial;

    bool                            statEnabled = false;

    RHI::FrameThreadData **         threadData = nullptr;
    int                             numRenderTaskThreads = 0;

    BE1::Array<const Material *>    shaderMaterialSources;
    BE1::Array<ShaderMaterial>      shaderMaterialTable;
    BE1::HashIndex                  shaderMaterialHash { 256, 256 };
    RHI::Buffer *                   shaderMaterialBuffer = nullptr;
    RHI::Buffer *                   shaderMaterialUploadBuffer = nullptr;
    RHI::ViewID                     shaderMaterialBufferView = RHI::ViewID::InvalidID;
    RHI::GPUResourceState           shaderMaterialBufferState = RHI::GPUResourceState::Undefined;
    uint32_t                        shaderMaterialBufferCapacity = 0;
    uint32_t                        shaderMaterialTableBindlessIndex = 0;
    bool                            shaderMaterialTableDirty = false;

    BE1::Array<SharedPtr<Material>> retainedMaterials;
    BE1::Array<SharedPtr<MeshInstance>> retainedMeshInstances;
    BE1::Array<SharedPtr<Font>> retainedFonts;
};

BE_INLINE void RenderFrameData::BeginFrame() {
    // 프론트 엔드에서 이번 프레임의 프레임 데이터를 사용하기 위해서는, 백엔드에서의 사용이 완료되야 한다.
    WaitFrameFence();

    statEnabled = false;

    // 스레드 별로 사용할 자원을 초기화한다.
    for (int32_t threadIndex = 0; threadIndex < numRenderTaskThreads; ++threadIndex) {
        threadData[threadIndex]->Reset();
    }

    frameSerial = ++globalFrameSerial;

    // 프레임 메모리를 초기화한다.
    BeginFrameMemAllocs();
}

BE_INLINE void RenderFrameData::EndFrame() {
    // 커맨드 버퍼에 커맨드의 끝을 기록.
    *(uint32_t *)(commands.buffer + commands.used) = static_cast<uint32_t>(RenderCommandId::End);

    // 커맨드 버퍼 비우기.
    commands.used = 0;
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
