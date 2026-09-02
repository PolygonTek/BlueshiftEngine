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

#include <atomic>

#include "GpuOcean.h"
#include "Math/Color3.h"
#include "Math/Vector3.h"

namespace BE1 {
    class Frustum;
    class OBB;
}

class ShaderPipeline;
class ShaderPipelineBootstrap;
class ShaderTemplate;
class Texture;

/*
---------------------------------------------------------------------------------------------------

    GPU Ocean Surface

---------------------------------------------------------------------------------------------------
*/

class GpuOceanSurface {
    friend class RenderSystem;

public:
    static constexpr uint32_t       GridResolution = 256;
    static constexpr uint32_t       GridLodCount = 3;
    static constexpr uint32_t       MaxGridTileCountPerAxis = 32;
    static constexpr uint32_t       MaxQueryCount = 4096;

    struct QueryReadbackFrame {
        enum class State : uint8_t {
            Idle,                   ///< query dispatch 가 이 frame 을 사용할 수 있는 상태
            Resolving,              ///< GPU 결과를 readback buffer 로 복사하는 command 를 기록한 상태
            Submitted               ///< command list 가 제출되어 fence 완료를 기다리는 상태
        };

        RHI::Buffer *               buffer = nullptr;
        uint32_t                    queryCount = 0;
        std::atomic<uint64_t>       fenceValue = 0;
        std::atomic<State>          state = State::Idle;

        void                        SetReadbackFenceValue(uint64_t fenceValue) { this->fenceValue.store(fenceValue); state.store(State::Submitted); }
    };

    struct Settings {
        float                       gridSpacing = 1.0f;
        uint32_t                    gridTileCountPerAxis = 16;
        float                       height = 0.0f;
        float                       maxDisplacement = 32.0f;        ///< CPU tile culling bounds 를 모든 축으로 확장하는 최대 수면 변위 (m)
        bool                        frustumCulling = true;

        bool                        wireframe = false;
        float                       roughness = 0.08f;
        float                       fresnelF0 = 0.02f;
        BE1::Color3                 waterColor = BE1::Color3(0.061f, 0.18f, 0.25f); ///< sRGB water body color
        bool                        refractionEnabled = true;
        float                       refractionDistance = 2.0f;      ///< 화면 공간 굴절 방향을 추적할 최대 거리 (m)
        float                       absorptionDistance = 30.0f;     ///< 투과율이 waterColor 에 도달하는 물 속 거리 (m)
        BE1::Color3                 scatterColor = BE1::Color3(0.0f, 0.34f, 0.38f); ///< 파도 마루에서 waterColor 를 기준으로 밝힐 sRGB 산란색
        float                       scatterHeight = 8.0f;           ///< 산란색이 최대 강도에 도달하는 기준 수면 위 높이 (m)
        BE1::Color3                 foamColor = BE1::Color3(0.955f, 0.978f, 1.0f); ///< sRGB foam color
        float                       foamDetailMapSize = 16.0f;      ///< foam detail map 이 world-space 에서 한 번 반복되는 크기 (m)
        float                       foamDetailStrength = 1.0f;      ///< foam history mask 에 foam detail map 을 적용하는 강도 [0, 1]
        float                       cascadeFadeScale = 20.0f;       ///< cascade patch size 에 곱해 거리 기반 fade 구간을 정하는 배율

        bool                        IsValid() const;
    };

    struct View {
        BE1::Vec3                   origin = BE1::Vec3::origin;
        const BE1::Frustum *        frustum = nullptr;
        const BE1::OBB *            box = nullptr;
        bool                        infiniteZFar = false;
    };

    struct DrawStats {
        uint32_t                    drawCount = 0;
        uint64_t                    triangleCount = 0;
        uint32_t                    instanceBufferAllocCount = 0;
        uint32_t                    instanceBufferUploadBytes = 0;
    };

    void                            Init(GpuOcean *gpuOcean);
    void                            Shutdown();
    void                            TrackBootstrapPipelines(ShaderPipelineBootstrap &bootstrap) const;

    const SharedPtr<ShaderTemplate> &GetShaderTemplate() const { return shaderTemplate; }
    bool                            IsGridReady() const { return gridIndexBuffer != nullptr; }

                                    /// 최대 MaxQueryCount 개의 query 를 GPU 에 기록하고 readback frame 을 반환한다.
    [[nodiscard]]                   /// 반환된 frame 의 fence 값은 command list 제출 시 backend 가 설정한다.
    QueryReadbackFrame *            DispatchQueries(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, const OceanSurfaceQueryInput *queries, uint32_t queryCount, float surfaceHeight);

                                    /// 완료된 query 결과 cache 에서 queryId 에 해당하는 최신 값을 반환한다.
    bool                            GetQueryResult(uint32_t queryId, OceanSurfaceQueryResult &outResult) const;

                                    /// 현재 ocean field 로 카메라 중심의 NxN 수면 grid 중 view volume 에 보이는 tile 을 렌더링한다.
    bool                            Draw(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, const RHI::PipelineState *pipelineState,
                                        const RHI::ConstantBuffer *frameConstantBuffer, const RHI::ConstantBuffer *cameraConstantBuffer, const Settings &settings, const View &view,
                                        const RHI::Texture *sceneColorTexture, const RHI::Texture *sceneDepthTexture, DrawStats *drawStats = nullptr) const;

private:
    struct GridLodRange {
        uint32_t                    firstIndex = 0;
        uint32_t                    indexCount = 0;
    };

    bool                            CreateGrid();
    void                            DestroyGrid();
    bool                            EnsureQueryResources();
    void                            DestroyQueryResources();
    void                            ConsumeCompletedQueries();

    GpuOcean *                      gpuOcean = nullptr;

    SharedPtr<ShaderTemplate>       shaderTemplate;
    SharedPtr<ShaderTemplate>       queryShaderTemplate;
    SharedPtr<ShaderPipeline>       queryPipeline;
    SharedPtr<Texture>              foamDetailTexture;
    RHI::IndexBuffer *              gridIndexBuffer = nullptr;
    GridLodRange                    gridLodRanges[GridLodCount] = {};

    RHI::Buffer *                   queryOutputBuffer = nullptr;
    RHI::ViewID                     queryOutputUAV = RHI::ViewID::InvalidID;
    QueryReadbackFrame              queryReadbackFrames[2] = {};
    uint32_t                        queryReadbackFrameIndex = 0;
    OceanSurfaceQueryResult         queryResultCache[MaxQueryCount] = {};
    bool                            queryResultValid[MaxQueryCount] = {};
};
