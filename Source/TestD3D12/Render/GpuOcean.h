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

#include "GpuFFT.h"
#include "Math/Color3.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"

namespace BE1 {
    class Frustum;
    class OBB;
}

class ShaderPipeline;
class ShaderTemplate;
class Texture;
struct OceanPhillipsInitialSpectrumParams;
struct OceanTMAInitialSpectrumParams;
struct OceanTimeSpectrumParams;
struct OceanResolveFieldsParams;

/*
---------------------------------------------------------------------------------------------------

    GPU Ocean

---------------------------------------------------------------------------------------------------
*/

class GpuOcean {
public:
    static constexpr uint32_t       CascadeCount = 3;
    static_assert(CascadeCount >= 2 && CascadeCount <= 4, "GpuOcean supports between two and four cascades");

    static constexpr uint32_t       FieldTextureSliceCount = 2 * CascadeCount;
    static constexpr uint32_t       SurfaceGridResolution = 256;
    static constexpr uint32_t       SurfaceGridLodCount = 3;
    static constexpr uint32_t       MaxSurfaceGridTileCountPerAxis = 32;

    struct SurfaceDrawStats {
        uint32_t                    drawCount = 0;
        uint64_t                    triangleCount = 0;
        uint32_t                    instanceBufferAllocCount = 0;
        uint32_t                    instanceBufferUploadBytes = 0;
    };

    struct SurfaceView {
        BE1::Vec3                   origin = BE1::Vec3::origin;
        const BE1::Frustum *        frustum = nullptr;
        const BE1::OBB *            box = nullptr;
        bool                        infiniteZFar = false;
    };

    enum class SpectrumModel : uint8_t {
        Phillips,                   ///< Phillips spectrum
        TMA                         ///< TMA spectrum + Hasselmann/isotropic directional spread (+swell parameter)
    };

    struct PhillipsSpectrumSettings {
        float                       spectrumScale = 0.001f;         ///< model-specific power spectrum scale
        float                       oppositeWaveDamping = 0.07f;    ///< 역풍 방향 파동에 적용하는 감쇠 배율 [0, 1]. 1이면 감쇠하지 않고 0 이면 완전히 제거한다.

        bool                        operator==(const PhillipsSpectrumSettings &rhs) const { return spectrumScale == rhs.spectrumScale && oppositeWaveDamping == rhs.oppositeWaveDamping; }
        bool                        operator!=(const PhillipsSpectrumSettings &rhs) const { return !(*this == rhs); }
    };

    struct TMASpectrumSettings {
        float                       spectrumScale = 1.0f;           ///< model-specific power spectrum scale
        float                       fetch = 250000.0f;              ///< 바람이 수면 위를 같은 방향으로 불어온 거리 (m). alpha 와 peakOmega 계산에 사용한다.
        float                       swell = 0.8f;                   ///< 저주파 파동의 진행 방향 집중도를 높이는 swell 계수 [0, 1]
        float                       spread = 0.2f;                  ///< Hasselmann 분포에 섞는 등방 분포의 비율 [0, 1]. 1이면 완전한 등방 분포다.

        bool                        operator==(const TMASpectrumSettings &rhs) const { return spectrumScale == rhs.spectrumScale && fetch == rhs.fetch && swell == rhs.swell && spread == rhs.spread; }
        bool                        operator!=(const TMASpectrumSettings &rhs) const { return !(*this == rhs); }
    };

    struct CascadeDesc {
        float                       patchSize = 0.0f;               ///< L
        float                       minWaveNumber = 0.0f;           ///< inclusive lower bound on |k|
        float                       maxWaveNumber = 0.0f;           ///< exclusive upper bound on |k|
        uint32_t                    randomSeed = 0;

        bool                        operator==(const CascadeDesc &rhs) const { return patchSize == rhs.patchSize && minWaveNumber == rhs.minWaveNumber && maxWaveNumber == rhs.maxWaveNumber && randomSeed == rhs.randomSeed; }
        bool                        operator!=(const CascadeDesc &rhs) const { return !(*this == rhs); }
    };

    struct CascadeSettings {
        float                       updateRate = 60.0f;             ///< 캐스케이드 field 갱신 빈도 (Hz). simulation time 을 기준으로 스케줄링한다.
        float                       foamThreshold = 0.5f;           ///< Jacobian 이 이 값보다 작을 때 foam 생성을 시작한다.
        float                       foamGenerationRate = 1.0f;      ///< Jacobian 압축량을 foam history 생성률로 변환하는 배율 (1/s)
        float                       foamHalfLife = 1.0f;            ///< 생성된 foam history 가 절반으로 감소하는 시간 (s)
    };

    struct SimulationSettings {
        enum class ChangeFlag : uint8_t {
            None                    = 0,
            RecreateResources       = BIT(0),
            RegenInitialSpectrum    = BIT(1),
            ResetFoamHistory        = BIT(2)
        };

        friend constexpr std::true_type EnableBitmaskEnum(ChangeFlag) noexcept { return {}; }

        GpuFFT::FFTSize             fftSize = GpuFFT::FFTSize::FFT512;
        float                       displacementScale = 1.0f;       ///< linear scale applied to the final displacement and derivatives
        float                       choppiness = 1.0f;              ///< horizontal displacement 배율. 클수록 파봉이 날카로워지지만 surface folding 이 발생할 수 있다.
        float                       timeScale = 1.0f;

        SpectrumModel               spectrumModel = SpectrumModel::TMA;
        PhillipsSpectrumSettings    phillips;
        TMASpectrumSettings         tma;
        BE1::Vec2                   windDirection = BE1::Vec2(1.0f, 0.0f); ///< 바람이 불어오는 방향
        float                       windSpeed = 20.0f;              ///< 해수면 위 10m 풍속 U10 (m/s). Phillips 와 TMA 의 fetch 기반 스펙트럼 및 방향 분포 계산에 사용한다.
        float                       gravity = 9.81f;                ///< 스펙트럼과 분산 관계 계산에 사용하는 중력 가속도 (m/s^2)
        float                       depth = 20.0f;                  ///< 유한 수심 분산 관계와 TMA 수심 보정에 사용하는 수심 (m)
        float                       smallWaveDamping = 0.01f;       ///< 짧은 파장을 exp(-k^2 * damping^2) 로 감쇠하는 길이 척도

        CascadeDesc                 cascades[CascadeCount];
        CascadeSettings             cascadeSettings[CascadeCount];  ///< cascades 와 같은 index 를 사용하는 spectrum 재생성 비의존 설정
        float                       cascadeBeta = 0.5f;             ///< high frequency boundary ratio

        bool                        foamEnabled = true;             ///< foam 생성, history 갱신 및 surface 합성을 활성화한다.

        SimulationSettings();

                                    /// 가장 큰 patch size 와 고주파 경계 비율로 각 cascade 의 patch size 와 파수 범위를 자동 초기화한다.
        void                        InitCascades(float largestPatchSize, float beta);

                                    /// spectrum model parameter 가 같은지 검사한다.
        bool                        HasSameSpectrumModelParameters(const SimulationSettings &rhs) const;

                                    /// rhs 설정을 적용할 때 필요한 GPU 상태 변경을 분류한다.
        ChangeFlag                  ClassifyChanges(const SimulationSettings &rhs) const;

        bool                        IsValid() const;
    };

    struct SurfaceSettings {
        float                       gridSpacing = 1.0f;
        uint32_t                    gridTileCountPerAxis = 16;
        float                       height = 0.0f;
        float                       maxDisplacement = 32.0f;        ///< CPU tile culling bounds 를 모든 축으로 확장하는 최대 수면 변위 (m)
        bool                        frustumCulling = true;

        bool                        wireframe = false;
        float                       roughness = 0.08f;
        float                       normalMapSize = 8.0f;           ///< detail normal map 이 world-space 에서 한 번 반복되는 크기 (m)
        float                       normalStrength = 0.35f;
        float                       normalScrollSpeedScale = 0.025f; ///< windSpeed 에 곱해 detail normal map 의 이동 속도를 정하는 배율
        BE1::Color3                 waterColor = BE1::Color3(0.061f, 0.215f, 0.271f); ///< sRGB water body color
        BE1::Color3                 foamColor = BE1::Color3(0.955f, 0.978f, 1.0f); ///< sRGB foam color
        float                       foamDetailMapSize = 16.0f;      ///< foam detail map 이 world-space 에서 한 번 반복되는 크기 (m)
        float                       foamDetailStrength = 1.0f;     ///< foam history mask 에 foam detail map 을 적용하는 강도 [0, 1]
        float                       fresnelF0 = 0.02f;
        float                       cascadeFadeScale = 20.0f;      ///< cascade patch size 에 곱해 거리 기반 fade 구간을 정하는 배율

        bool                        IsValid() const;
    };

    struct Settings {
        bool                        enabled = true;
        SimulationSettings          simulation;
        SurfaceSettings             surface;

        bool                        IsValid() const { return simulation.IsValid() && surface.IsValid(); }
    };

    void                            Init();
    void                            Shutdown();

    void                            UpdatePipelineBuildState();

    bool                            ArePipelinesReady() const { return pipelinesReady; }
    bool                            HasPipelineBuildFailed() const { return pipelineBuildFailed; }

    const SharedPtr<ShaderTemplate> &GetSurfaceShaderTemplate() const { return surfaceShaderTemplate; }

    bool                            IsSurfaceGridReady() const { return surfaceGridIndexBuffer != nullptr; }

                                    /// 현재 모든 cascade 의 출력 field (displacement/derivative textures) 가 준비되었는지 여부를 리턴한다.
    bool                            AreCascadeOutputsReady() const { return cascadeOutputsReady; }

                                    /// 짝수 slice 에 displacement, 홀수 slice 에 derivative 를 저장하는 field texture array 를 얻는다.
    const RHI::Texture *            GetFieldTextureArray() const { return fieldTextureArray; }

                                    /// revision 이 갱신된 시뮬레이션 설정을 내부 상태에 적용한다.
    void                            ApplySimulationSettings(const SimulationSettings &newSettings, uint64_t revision);

                                    /// 현재 시뮬레이션 설정으로 모든 cascade 의 출력 field 를 갱신한다.
    bool                            Update(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, float deltaTime);

                                    /// 현재 cascade 출력으로 카메라 중심의 NxN 수면 grid 중 view volume 에 보이는 tile 을 렌더링한다.
    bool                            DrawSurface(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, const RHI::PipelineState *pipelineState,
                                        const RHI::ConstantBuffer *frameConstantBuffer, const RHI::ConstantBuffer *cameraConstantBuffer, const SurfaceSettings &surfaceSettings, const SurfaceView &surfaceView,
                                        SurfaceDrawStats *drawStats = nullptr) const;

private:
    struct SurfaceGridLodRange {
        uint32_t                    firstIndex = 0;
        uint32_t                    indexCount = 0;
    };

    bool                            HasResources() const;
    bool                            EnsureResources();
    void                            DestroyResources();
    bool                            CreateSurfaceGrid();
    void                            DestroySurfaceGrid();
    void                            ResetCascadeUpdateSchedule();

                                    /// 각 cascade 별로 displacement, derivative, foam history texture 를 업데이트한다.
    bool                            UpdateCascades(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, const OceanTimeSpectrumParams &baseParams, const OceanResolveFieldsParams &resolveParams);

                                    /// Phillips spectrum 으로 모든 cascade 의 초기 스펙트럼을 하나의 Z-axis dispatch 로 생성한다.
    bool                            GenerateInitialSpectraPhillips(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, const OceanPhillipsInitialSpectrumParams &commonParams);

                                    /// TMA spectrum 과 Hasselmann/isotropic directional spread 혼합으로 모든 cascade 의 초기 스펙트럼을 하나의 Z-axis dispatch 로 생성한다.
    bool                            GenerateInitialSpectraTMA(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, const OceanTMAInitialSpectrumParams &commonParams);

                                    /// 초기 스펙트럼을 시간에 따라 진화시키고 IFFT 입력용 complex spectrum 을 생성한다.
                                    /// 멤버 initialSpectrumTextureArray 는 짝수 정사각형 크기의 RGBA16F/RGBA32F 2D texture array 여야 한다.
                                    /// 멤버 timeSpectrumBuffer 는 N * N * OceanSpectrumComplexPlaneCount 개의 float2 를 저장하는 structured buffer 여야 한다.
                                    /// 각 complex plane 은 두 real field spectrum 을 Z = A + iB 로 패킹하며 일반 row-major 순서로 저장한다.
                                    /// 두 리소스의 primary view 를 사용하며 호출 전후 각각 ShaderResourceCompute, Undefined(Common) 상태이다.
    bool                            GenerateTimeSpectrum(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, const OceanTimeSpectrumParams &params);

                                    /// transposed complex IFFT 결과를 displacement/derivative map 으로 변환하고 foam history 를 갱신한다.
                                    /// 멤버 spatialFieldsBuffer 는 N * N * OceanSpectrumComplexPlaneCount 개의 float2 를 저장하는 structured buffer 여야 한다.
                                    /// fieldTextureArray 는 RGBA16F/RGBA32F 의 2 * CascadeCount slice, foamHistoryTextureArray 는 R16F/R32F 의 CascadeCount slice 여야 한다.
                                    /// 세 리소스의 primary view 를 사용하며 호출 전후 spatialFieldsBuffer 는 Undefined(Common), texture 들은 ShaderResource 상태이다.
    bool                            ResolveOceanFields(RHI::CommandList *commandList, const RHI::ConstantBuffer *resolveConstantBuffer);

    SharedPtr<ShaderTemplate>       generatePhillipsInitialSpectrumShaderTemplate;
    SharedPtr<ShaderTemplate>       generateTMAInitialSpectrumShaderTemplate;
    SharedPtr<ShaderTemplate>       generateTimeSpectrumShaderTemplate;
    SharedPtr<ShaderTemplate>       resolveOceanFieldsShaderTemplate;
    SharedPtr<ShaderTemplate>       surfaceShaderTemplate;
    SharedPtr<Texture>              foamDetailTexture;
    SharedPtr<Texture>              waterDetailNormalTexture;

    SharedPtr<ShaderPipeline>       generatePhillipsInitialSpectrumPipeline;
    SharedPtr<ShaderPipeline>       generateTMAInitialSpectrumPipeline;
    SharedPtr<ShaderPipeline>       generateTimeSpectrumPipeline;
    SharedPtr<ShaderPipeline>       resolveOceanFieldsPipeline;

    SimulationSettings              simulationSettings;
    uint64_t                        appliedSimulationSettingsRevision = 0;

    RHI::Buffer *                   timeSpectrumBuffer = nullptr;           // (개별 캐스케이드 계산에 임시로 사용됨) 시간에 따라 진화된 complex spectrum 을 저장하는 structured buffer
    RHI::Buffer *                   ifftWorkBuffer = nullptr;               // (개별 캐스케이드 계산에 임시로 사용됨) IFFT 를 위한 work buffer
    RHI::Buffer *                   spatialFieldsBuffer = nullptr;          // (개별 캐스케이드 계산에 임시로 사용됨) IFFT 결과를 저장하는 structured buffer

    RHI::Texture *                  initialSpectrumTextureArray = nullptr;  // slice per cascade, RG: h0(k), BA: conj(h0(-k))
    RHI::Texture *                  fieldTextureArray = nullptr;            // even slices: displacement, odd slices: derivative
    RHI::Texture *                  foamHistoryTextureArray = nullptr;      // slice per cascade, R: temporally accumulated foam

    BE1::Array<RHI::ViewID>         fieldMipSRVs;
    BE1::Array<RHI::ViewID>         fieldMipUAVs;
    BE1::Array<RHI::ViewID>         foamMipSRVs;
    BE1::Array<RHI::ViewID>         foamMipUAVs;

    RHI::IndexBuffer *              surfaceGridIndexBuffer = nullptr;
    SurfaceGridLodRange             surfaceGridLodRanges[SurfaceGridLodCount] = {};

    float                           simulationTime = 0.0f;
    float                           cascadeLastUpdateTimes[CascadeCount] = {};
    float                           cascadeNextUpdateTimes[CascadeCount] = {};
    float                           cascadeScheduledUpdateRates[CascadeCount] = {};
    uint32_t                        cascadeUpdateInitializedMask = 0u;
    uint32_t                        foamHistoryMayContainFoamMask = 0u; ///< reset 이후 foam source 가 생성될 수 있었던 cascade mask

    bool                            cascadeResourcesDirty = true;
    bool                            initialSpectraReady = false;
    bool                            cascadeOutputsReady = false;
    bool                            foamHistoryResetPending = true;
    bool                            pipelinesReady = false;
    bool                            pipelineBuildFailed = false;
};
