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
#include "Math/Vector2.h"

class ShaderPipeline;
class ShaderTemplate;
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
    static constexpr uint32_t       SurfaceGridIndexCount = (SurfaceGridResolution - 1) * (SurfaceGridResolution - 1) * 6;
    static constexpr uint32_t       MaxSurfaceGridTileCountPerAxis = 16;

    enum class SpectrumModel : uint8_t {
        Phillips,
        TMA
    };

    struct PhillipsSpectrumSettings {
        float                       spectrumScale = 0.001f;     ///< model-specific power spectrum scale
        float                       oppositeWaveDamping = 0.07f;///< 역풍 방향 파동에 적용하는 감쇠 배율 [0, 1]. 1이면 감쇠하지 않고 0 이면 완전히 제거한다.

        bool                        operator==(const PhillipsSpectrumSettings &rhs) const { return spectrumScale == rhs.spectrumScale && oppositeWaveDamping == rhs.oppositeWaveDamping; }
        bool                        operator!=(const PhillipsSpectrumSettings &rhs) const { return !(*this == rhs); }
    };

    struct TMASpectrumSettings {
        float                       spectrumScale = 1.0f;       ///< model-specific power spectrum scale
        float                       alpha = 0.0081f;            ///< JONSWAP 스펙트럼의 무차원 에너지 계수
        float                       peakOmega = 0.8f;           ///< 스펙트럼 에너지가 최대가 되는 peak 각주파수 (rad/s)
        float                       swell = 0.0f;               ///< 저주파 파동의 진행 방향 집중도를 높이는 swell 계수 [0, 1]

        bool                        operator==(const TMASpectrumSettings &rhs) const { return spectrumScale == rhs.spectrumScale && alpha == rhs.alpha && peakOmega == rhs.peakOmega && swell == rhs.swell; }
        bool                        operator!=(const TMASpectrumSettings &rhs) const { return !(*this == rhs); }
    };

    struct CascadeDesc {
        float                       patchSize = 0.0f;           ///< L
        float                       minWaveNumber = 0.0f;       ///< inclusive lower bound on |k|
        float                       maxWaveNumber = 0.0f;       ///< exclusive upper bound on |k|
        uint32_t                    randomSeed = 0;

        bool                        operator==(const CascadeDesc &rhs) const { return patchSize == rhs.patchSize && minWaveNumber == rhs.minWaveNumber && maxWaveNumber == rhs.maxWaveNumber && randomSeed == rhs.randomSeed; }
        bool                        operator!=(const CascadeDesc &rhs) const { return !(*this == rhs); }
    };

    struct Settings {
        bool                        enabled = true;
        float                       surfaceHeight = 0.0f;
        float                       surfaceGridSpacing = 2.0f;
        uint32_t                    surfaceGridTileCountPerAxis = 3;
        float                       surfaceRoughness = 0.08f;

        float                       waveAmplitude = 1.0f;       ///< linear scale applied to the final displacement and derivatives
        SpectrumModel               spectrumModel = SpectrumModel::TMA;
        PhillipsSpectrumSettings    phillips;
        TMASpectrumSettings         tma;
        GpuFFT::FFTSize             fftSize = GpuFFT::FFTSize::FFT256;
        float                       cascadeBeta = 0.5f;         ///< high frequency boundary ratio
        CascadeDesc                 cascades[CascadeCount];

        BE1::Vec2                   windDirection = BE1::Vec2(1.0f, 0.0f);
        float                       windSpeed = 10.0f;          ///< 풍속 (m/s). Phillips 의 장파장 억제와 TMA 의 방향 분포 계산에 사용한다.
        float                       gravity = 9.8f;             ///< 스펙트럼과 분산 관계 계산에 사용하는 중력 가속도 (m/s^2)
        float                       depth = 100.0f;             ///< 유한 수심 분산 관계와 TMA 수심 보정에 사용하는 수심 (m)
        float                       smallWaveDamping = 0.01f;   ///< 짧은 파장을 exp(-k^2 * damping^2) 로 감쇠하는 길이 척도
        float                       choppiness = 1.0f;          ///< horizontal displacement 배율. 클수록 파봉이 날카로워지지만 surface folding 이 발생할 수 있다.
        float                       foamThreshold = 0.8f;       ///< Jacobian 이 이 값보다 작을 때 foam 생성을 시작한다.
        float                       foamGain = 2.0f;            ///< Jacobian 압축량을 새 foam 양으로 변환하는 배율
        float                       foamDecayRate = 0.5f;       ///< foam history 의 지수 감쇠율 (1/s)
        float                       foamAddRate = 1.0f;         ///< new foam 의 선형 생성률 (1/s)
        float                       timeScale = 1.0f;

        Settings() { InitCascades(1024.0f, 0.5f); }

                                    /// 가장 큰 patch size 와 고주파 경계 비율로 각 cascade 의 patch size 와 파수 범위를 자동 초기화한다.
        void                        InitCascades(float largestPatchSize, float beta);
    };

    void                            Init();
    void                            Shutdown();

    void                            UpdatePipelineBuildState();

    bool                            ArePipelinesReady() const { return pipelinesReady; }
    bool                            HasPipelineBuildFailed() const { return pipelineBuildFailed; }

    const SharedPtr<ShaderTemplate> &GetSurfaceShaderTemplate() const { return surfaceShaderTemplate; }
    uint32_t                        GetSurfaceGridIndexCount() const { return surfaceGridIndexBuffer ? SurfaceGridIndexCount : 0; }
    static uint32_t                 GetSurfaceGridInstanceCount(const Settings &settings) { return settings.surfaceGridTileCountPerAxis * settings.surfaceGridTileCountPerAxis; }

                                    /// 주어진 index 의 cascade 설정을 얻는다.
    const CascadeDesc *             GetCascadeDesc(uint32_t cascadeIndex) const { return cascadeIndex < CascadeCount ? &settings.cascades[cascadeIndex] : nullptr; }

                                    /// 현재 모든 cascade 의 출력 field (displacement/derivative textures) 가 준비되었는지 여부를 리턴한다.
    bool                            AreCascadeOutputsReady() const { return cascadeOutputsReady; }

                                    /// 짝수 slice 에 displacement, 홀수 slice 에 derivative 를 저장하는 field texture array 를 얻는다.
    const RHI::Texture *            GetFieldTextureArray() const { return fieldTextureArray; }

                                    /// 설정 변경을 적용하고 모든 cascade 의 출력 field 를 갱신한다.
                                    /// enabled 가 false 이면 기존 리소스와 출력은 유지하고 시뮬레이션을 진행하지 않는다.
    bool                            Update(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, const Settings &newSettings, float deltaTime);

                                    /// 현재 cascade 출력으로 카메라 중심의 NxN 수면 grid tile 을 렌더링한다.
    bool                            DrawSurface(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, const RHI::PipelineState *pipelineState,
                                        const RHI::ConstantBuffer *frameConstantBuffer, const RHI::ConstantBuffer *cameraConstantBuffer, const Settings &renderSettings) const;

private:
    bool                            HasResources() const;
    bool                            EnsureResources();
    void                            DestroyResources();
    bool                            CreateSurfaceGrid();
    void                            DestroySurfaceGrid();

    bool                            InitialSpectrumSettingsEqual(const Settings &lhs, const Settings &rhs) const;

                                    /// 각 cascade 별로 displacement, derivative, foam history texture 를 업데이트한다.
    bool                            UpdateCascades(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, const OceanTimeSpectrumParams &baseParams, const OceanResolveFieldsParams &resolveParams);

                                    /// Phillips spectrum 으로 초기 스펙트럼을 생성한다.
                                    /// outputTextureArray 는 짝수 정사각형 크기의 RGBA16F/RGBA32F 2D texture array 여야 한다.
                                    /// outputUAV 는 모든 cascade slice 의 mip 0 을 포함해야 하며, texture array 는 호출 전후 UnorderedAccess 상태이다.
    bool                            GenerateInitialSpectrumPhillips(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, const OceanPhillipsInitialSpectrumParams &params, RHI::Texture *outputTextureArray, RHI::ViewID outputUAV = RHI::ViewID::PrimaryID);
                                    /// Phillips spectrum 으로 모든 cascade 의 초기 스펙트럼을 생성한다.
    bool                            GenerateInitialSpectraPhillips(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, const OceanPhillipsInitialSpectrumParams &baseParams);

                                    /// TMA spectrum 과 Hasselmann directional spread 로 초기 스펙트럼을 생성한다.
                                    /// outputTextureArray 는 짝수 정사각형 크기의 RGBA16F/RGBA32F 2D texture array 여야 한다.
                                    /// outputUAV 는 모든 cascade slice 의 mip 0 을 포함해야 하며, texture array 는 호출 전후 UnorderedAccess 상태이다.
    bool                            GenerateInitialSpectrumTMA(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, const OceanTMAInitialSpectrumParams &params, RHI::Texture *outputTextureArray, RHI::ViewID outputUAV = RHI::ViewID::PrimaryID);
                                    /// TMA spectrum 과 Hasselmann directional spread 로 모든 cascade 의 초기 스펙트럼을 생성한다.
    bool                            GenerateInitialSpectraTMA(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, const OceanTMAInitialSpectrumParams &baseParams);

                                    /// 초기 스펙트럼을 시간에 따라 진화시키고 IFFT 입력용 complex spectrum 을 생성한다.
                                    /// initialSpectrumTextureArray 는 짝수 정사각형 크기의 RGBA16F/RGBA32F 2D texture array 여야 한다.
                                    /// outputSpectrumBuffer 는 N * N * OceanSpectrumComplexPlaneCount 개의 float2 를 저장하는 structured buffer 여야 한다.
                                    /// 각 complex plane 은 두 real field spectrum 을 Z = A + iB 로 패킹하며 일반 row-major 순서로 저장한다.
                                    /// initialSpectrumSRV 는 호출 전후 ShaderResourceCompute 상태이고, outputUAV 는 호출 전후 Undefined(Common) 상태이다.
    bool                            GenerateTimeSpectrum(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, const OceanTimeSpectrumParams &params, const RHI::Texture *initialSpectrumTextureArray, RHI::Buffer *outputSpectrumBuffer, RHI::ViewID initialSpectrumSRV = RHI::ViewID::PrimaryID, RHI::ViewID outputUAV = RHI::ViewID::PrimaryID);

                                    /// transposed complex IFFT 결과를 displacement/derivative map 으로 변환하고 foam history 를 갱신한다.
                                    /// spatialFieldsBuffer 는 N * N * OceanSpectrumComplexPlaneCount 개의 float2 를 저장하는 structured buffer 여야 한다.
                                    /// field texture array 는 RGBA16F/RGBA32F 의 2 * CascadeCount slice, foam history texture array 는 R16F/R32F 의 CascadeCount slice 여야 한다.
                                    /// spatialFieldsBuffer 는 호출 전후 Undefined(Common) 상태이고, output texture 들은 호출 전후 ShaderResource 상태이다.
    bool                            ResolveOceanFields(RHI::CommandList *commandList, const RHI::ConstantBuffer *resolveConstantBuffer, uint32_t cascadeIndex,
                                        RHI::Buffer *spatialFieldsBuffer, RHI::Texture *outputFieldTextureArray, RHI::Texture *foamHistoryTextureArray,
                                        RHI::ViewID spatialFieldsSRV = RHI::ViewID::PrimaryID, RHI::ViewID outputFieldUAV = RHI::ViewID::PrimaryID,
                                        RHI::ViewID foamHistoryUAV = RHI::ViewID::PrimaryID);

    SharedPtr<ShaderTemplate>       generatePhillipsInitialSpectrumShaderTemplate;
    SharedPtr<ShaderTemplate>       generateTMAInitialSpectrumShaderTemplate;
    SharedPtr<ShaderTemplate>       generateTimeSpectrumShaderTemplate;
    SharedPtr<ShaderTemplate>       resolveOceanFieldsShaderTemplate;
    SharedPtr<ShaderTemplate>       surfaceShaderTemplate;

    SharedPtr<ShaderPipeline>       generatePhillipsInitialSpectrumPipeline;
    SharedPtr<ShaderPipeline>       generateTMAInitialSpectrumPipeline;
    SharedPtr<ShaderPipeline>       generateTimeSpectrumPipeline;
    SharedPtr<ShaderPipeline>       resolveOceanFieldsPipeline;

    Settings                        settings;

    RHI::Buffer *                   timeSpectrumBuffer = nullptr;   // (개별 캐스캐이드 계산에 임시로 사용됨) 시간에 따라 진화된 complex spectrum 을 저장하는 structured buffer
    RHI::Buffer *                   ifftWorkBuffer = nullptr;       // (개별 캐스캐이드 계산에 임시로 사용됨) IFFT 를 위한 work buffer
    RHI::Buffer *                   spatialFieldsBuffer = nullptr;  // (개별 캐스캐이드 계산에 임시로 사용됨) IFFT 결과를 저장하는 structured buffer

    RHI::Texture *                  initialSpectrumTextureArray = nullptr; // slice per cascade, RG: h0(k), BA: conj(h0(-k))
    RHI::Texture *                  fieldTextureArray = nullptr; // even slices: displacement, odd slices: derivative
    RHI::Texture *                  foamHistoryTextureArray = nullptr; // slice per cascade, R: temporally accumulated foam
    BE1::Array<RHI::ViewID>         fieldMipSRVs;
    BE1::Array<RHI::ViewID>         fieldMipUAVs;
    BE1::Array<RHI::ViewID>         foamMipSRVs;
    BE1::Array<RHI::ViewID>         foamMipUAVs;

    RHI::IndexBuffer *              surfaceGridIndexBuffer = nullptr;

    float                           simulationTime = 0.0f;

    bool                            cascadeResourcesDirty = true;
    bool                            initialSpectraReady = false;
    bool                            cascadeOutputsReady = false;
    bool                            foamHistoryResetPending = true;
    bool                            pipelinesReady = false;
    bool                            pipelineBuildFailed = false;
};
