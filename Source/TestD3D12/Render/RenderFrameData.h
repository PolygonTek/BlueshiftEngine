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

#include "RHI/RHI.h"
#include "RenderLight.h"
#include "ShaderLightCullingTable.h"
#include "ShaderLightTable.h"
#include "ShaderMaterialTable.h"
#include "ShaderSurfaceTable.h"

class RenderContext;
class Texture;
class Material;
class MeshInstance;
class Font;
struct VisCamera;
struct VisObject;
struct VisLight;
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
    GenerateConvolutionBloomKernel,
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
    RHI::PipelineState *            imagePSO;
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

struct GenerateConvolutionBloomKernelRenderCommand {
    RenderCommandId                 commandId;
    float                           diffractionPSFScale;
    float                           dispersionAmount;
};

struct SwapBuffersRenderCommand {
    RenderCommandId                 commandId;
};

struct EndRenderCommand {
    RenderCommandId                 commandId;
};

struct PendingImGuiOverlay {
    RHI::ImGuiOverlay *             overlay = nullptr;
    void *                          userData = nullptr;
};

/*
---------------------------------------------------------------------------------------------------

    Render Frame Data

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
    struct Chunk {
        Chunk *                     next = nullptr;
        void *                      elements = nullptr;
        uint32_t                    count = 0;
    };

    template<typename T>
    class ChunkIterator {
    public:
        ChunkIterator() = default;
        explicit ChunkIterator(Chunk *chunk) { SetChunk(chunk); }

        T &                         operator*() const { return *current; }
        T *                         operator->() const { return current; }

        ChunkIterator &             operator++();

        bool                        operator==(const ChunkIterator &rhs) const { return current == rhs.current; }
        bool                        operator!=(const ChunkIterator &rhs) const { return current != rhs.current; }

    private:
        void                        SetChunk(Chunk *chunk);

        T *                         current = nullptr;
        T *                         end = nullptr;
        Chunk *                     nextChunk = nullptr;
    };

    template<typename T>
    class ChunkRange {
    public:
        explicit ChunkRange(Chunk *firstChunk) : firstChunk(firstChunk) {}

        ChunkIterator<T>            begin() const { return ChunkIterator<T>(firstChunk); }
        ChunkIterator<T>            end() const { return ChunkIterator<T>(); }

    private:
        Chunk *                     firstChunk;
    };

    void                            Init(int32_t frameIndex, int32_t numFrames);
    void                            Shutdown();

                                    // 이번 프레임 동안 사용할 임시 시스템 메모리를 할당한다.
                                    // 할당된 메모리는 다음 프레임에서 재사용될 때 일괄적으로 해제된다.
                                    // 현재는 프론트엔드에서 이번 프레임의 렌더 데이터를 구성하는 동안에만 사용한다.
                                    // 프론트엔드/백엔드 또는 여러 스레드에서 동시에 할당하려면 별도의 동기화가 필요하다.
    [[nodiscard]] void *            MemAlloc(int size);
    [[nodiscard]] void *            ClearedMemAlloc(int size);

    uint32_t                        NumVisCameras() const { return numVisCameras; }
    VisCamera *                     GetVisCameras() const { return visCameras; }
    [[nodiscard]] VisCamera *       AllocVisCamera();

    uint32_t                        NumVisObjects() const { return numVisObjects; }
    ChunkRange<VisObject>           GetVisObjects() const { return ChunkRange<VisObject>(firstVisObjectChunk); }
    [[nodiscard]] VisObject *       AllocVisObject();

    uint32_t                        NumVisLights() const { return numVisLights; }
    uint32_t                        NumVisPointLights() const { return numVisPointLights; }
    uint32_t                        NumVisSpotLights() const { return numVisSpotLights; }
    uint32_t                        NumVisBoxDirLights() const { return numVisBoxDirLights; }
    ChunkRange<VisLight>            GetVisLights() const { return ChunkRange<VisLight>(firstVisLightChunk); }
    [[nodiscard]] VisLight *        AllocVisLight(RenderLight::Type type);
    [[nodiscard]] uint32_t          ReserveLightCullingZBins();
    uint32_t                        NumLightCullingZBins() const { return numLightCullingZBins; }
    [[nodiscard]] uint32_t          ReserveLightCullingWords(uint32_t count);
    uint32_t                        NumLightCullingWords() const { return numLightCullingWords; }

                                    // 렌더 커맨드 버퍼 리턴
    RenderCommandBuffer *           GetCommands() { return &commands; }

                                    // 렌더 커맨드들..
    void                            CmdBeginContext(RenderContext *context);
    void                            CmdDrawCamera(const VisCamera *camera);
    void                            CmdSwapBuffers();
    void                            CmdScreenshot(int x, int y, int width, int height, const char *filename);
    void                            CmdGenerateConvolutionBloomKernel(float diffractionPSFScale, float dispersionAmount);
    void                            CmdEnd();

    bool                            HasPendingImGuiOverlay() const { return pendingImGuiOverlay.overlay != nullptr; }
    void                            SetPendingImGuiOverlay(RHI::ImGuiOverlay *overlay, void *userData);
    void                            RenderPendingImGuiOverlay(RHI::CommandList *commandList);
    void                            ClearPendingImGuiOverlay();

                                    // 렌더 태스크의 스레드 별 데이터 얻기
    RHI::FrameThreadData *          GetThreadData(int32_t threadIndex) { assert(threadIndex >= 0 && threadIndex < numRenderTaskThreads); return threadData[threadIndex]; }

    float                           GetFrameTime() const { return frameTime; }
    float                           GetElapsedTime() const { return elapsedTime; }
    float                           GetElapsedPrevTime() const { return elapsedPrevTime; }
    uint32_t                        GetFrameCount() const { return frameCount; }
    void                            SetFrameTiming(float frameTime, float elapsedTime, float elapsedPrevTime, uint32_t frameCount);

    bool                            IsStatEnabled() const { return statEnabled; }
    void                            SetStatEnabled(bool enabled) { statEnabled = enabled; }

    Material *                      RetainMaterial(const SharedPtr<Material> &material);
    MeshInstance *                  RetainMeshInstance(const SharedPtr<MeshInstance> &meshInstance);
    Font *                          RetainFont(const SharedPtr<Font> &font);
    Texture *                       RetainTexture(const SharedPtr<Texture> &texture);

    uint32_t                        GetShaderMaterialTableBindlessIndex() const { return shaderMaterialTable.GetBindlessIndex(); }
    uint32_t                        GetShaderSurfaceTableBindlessIndex() const { return shaderSurfaceTable.GetBindlessIndex(); }
    uint32_t                        GetShaderPointLightTableBindlessIndex() const { return shaderLightTable.GetPointLightBindlessIndex(); }
    uint32_t                        GetShaderSpotLightTableBindlessIndex() const { return shaderLightTable.GetSpotLightBindlessIndex(); }
    uint32_t                        GetShaderBoxDirLightTableBindlessIndex() const { return shaderLightTable.GetBoxDirLightBindlessIndex(); }
    uint32_t                        GetShaderLightRefTableBindlessIndex() const { return shaderLightCullingTable.GetLightRefBindlessIndex(); }
    uint32_t                        GetShaderLightZBinTableBindlessIndex() const { return shaderLightCullingTable.GetLightZBinBindlessIndex(); }
    uint32_t                        GetShaderLightTileWordTableBindlessIndex() const { return shaderLightCullingTable.GetLightTileWordBindlessIndex(); }

                                    // 셰이더 머티리얼 테이블에 머티리얼을 등록하고, 테이블 인덱스를 리턴한다.
    uint32_t                        RegisterShaderMaterial(const Material *material) { return shaderMaterialTable.Register(material); }

    bool                            UploadShaderMaterialTable(RHI::FrameThreadData *frameThreadData, RHI::CommandList *commandList) { return shaderMaterialTable.Upload(frameThreadData, commandList, true); }
    bool                            UploadShaderLightTable(RHI::FrameThreadData *frameThreadData, RHI::CommandList *commandList) { return shaderLightTable.Upload(frameThreadData, commandList, this, true); }
    bool                            UploadShaderLightCullingTable(RHI::FrameThreadData *frameThreadData, RHI::CommandList *commandList) { return shaderLightCullingTable.Upload(frameThreadData, commandList, this, true); }
    bool                            DispatchLightCulling(RHI::FrameThreadData *frameThreadData, RHI::CommandList *commandList, RHI::PipelineState *pipelineState) { return shaderLightCullingTable.DispatchLightCulling(frameThreadData, commandList, pipelineState, this); }
    bool                            UploadShaderSurfaceTable(RHI::FrameThreadData *frameThreadData, RHI::CommandList *commandList, const VisCamera *visCamera) { return shaderSurfaceTable.Upload(frameThreadData, commandList, visCamera, false); }
    bool                            EnsureShaderSurfaceTableCapacity() { return shaderSurfaceTable.EnsureBufferCapacity(maxShaderSurfaceCount); }

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
    [[nodiscard]] MemBlock *        AllocMemBlock();

                                    // 이번 프레임의 임시 시스템 메모리 할당 초기화 (메인 스레드에서 호출됨)
    void                            BeginFrameMemAllocs();

    void                            InitVisCameras(uint32_t maxVisCameras);
    void                            FreeVisCameras();
    void                            FreeVisObjects();
    void                            FreeVisLights();

    [[nodiscard]] Chunk *           AllocChunk(uint32_t elementSize, uint32_t elementCount);

    void                            ClearRetainedMaterials();
    void                            ClearRetainedMeshInstances();
    void                            ClearRetainedFonts();
    void                            ClearRetainedTextures();

    [[nodiscard]] void *            GetCommandBuffer(uint32_t bytes);

    MemBlock *                      headBlock = nullptr;
    MemBlock *                      currentBlock = nullptr;

    uint32_t                        maxVisCameras = 0;
    uint32_t                        numVisCameras = 0;
    VisCamera *                     visCameras = nullptr;

    uint32_t                        numVisObjects = 0;
    Chunk *                         firstVisObjectChunk = nullptr;
    Chunk *                         lastVisObjectChunk = nullptr;

    uint32_t                        numVisLights = 0;
    uint32_t                        numVisPointLights = 0;
    uint32_t                        numVisSpotLights = 0;
    uint32_t                        numVisBoxDirLights = 0;
    Chunk *                         firstVisLightChunk = nullptr;
    Chunk *                         lastVisLightChunk = nullptr;

    uint32_t                        numLightCullingZBins = 0;
    uint32_t                        numLightCullingWords = 0;

    RenderCommandBuffer             commands;
    PendingImGuiOverlay             pendingImGuiOverlay;

    FrameSyncState                  frameSyncState = FrameSyncState::RenderCompleted;
    uint64_t                        frameFenceValue = 0;
    uint64_t                        frameSerial = 0;
    static uint64_t                 globalFrameSerial;
    float                           frameTime = 0.0f;
    float                           elapsedTime = 0.0f;
    float                           elapsedPrevTime = 0.0f;
    uint32_t                        frameCount = 0;

    bool                            statEnabled = false;

    uint32_t                        maxShaderSurfaceCount = 0;

    RHI::FrameThreadData **         threadData = nullptr;
    int                             numRenderTaskThreads = 0;

    ShaderMaterialTable             shaderMaterialTable;
    ShaderLightTable                shaderLightTable;
    ShaderLightCullingTable         shaderLightCullingTable;
    ShaderSurfaceTable              shaderSurfaceTable;

    BE1::Array<SharedPtr<Material>> retainedMaterials;
    BE1::Array<SharedPtr<MeshInstance>> retainedMeshInstances;
    BE1::Array<SharedPtr<Font>>     retainedFonts;
    BE1::Array<SharedPtr<Texture>>  retainedTextures;
};

template<typename T>
BE_INLINE RenderFrameData::ChunkIterator<T> &RenderFrameData::ChunkIterator<T>::operator++() {
    current++;
    if (current == end) {
        SetChunk(nextChunk);
    }
    return *this;
}

template<typename T>
BE_INLINE void RenderFrameData::ChunkIterator<T>::SetChunk(RenderFrameData::Chunk *chunk) {
    if (!chunk) {
        current = nullptr;
        end = nullptr;
        nextChunk = nullptr;
        return;
    }

    current = static_cast<T *>(chunk->elements);
    end = current + chunk->count;
    nextChunk = chunk->next;
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

BE_INLINE void RenderFrameData::CmdGenerateConvolutionBloomKernel(float diffractionPSFScale, float dispersionAmount) {
    GenerateConvolutionBloomKernelRenderCommand *cmd = (GenerateConvolutionBloomKernelRenderCommand *)GetCommandBuffer(sizeof(GenerateConvolutionBloomKernelRenderCommand));
    if (!cmd) {
        return;
    }

    cmd->commandId = RenderCommandId::GenerateConvolutionBloomKernel;
    cmd->diffractionPSFScale = diffractionPSFScale;
    cmd->dispersionAmount = dispersionAmount;
}

BE_INLINE void RenderFrameData::CmdEnd() {
    EndRenderCommand *cmd = (EndRenderCommand *)GetCommandBuffer(sizeof(EndRenderCommand));
    if (!cmd) {
        return;
    }

    cmd->commandId = RenderCommandId::End;
}

BE_INLINE void RenderFrameData::SetFrameTiming(float frameTime, float elapsedTime, float elapsedPrevTime, uint32_t frameCount) {
    this->frameTime = frameTime;
    this->elapsedTime = elapsedTime;
    this->elapsedPrevTime = elapsedPrevTime;
    this->frameCount = frameCount;
}
