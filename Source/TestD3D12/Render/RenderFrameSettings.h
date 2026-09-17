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

#include <cstdint>

enum RenderBloomMode {
    Off = 0,
    Pyramid = 1,
    Convolution = 2
};

enum class RenderAOMode : uint32_t {
    Off = 0,
    SSAO = 1,
    GTAO = 2
};

// 메인 스레드에서 프레임 시작 시 생성되고, 해당 프레임 동안 변경되지 않는 렌더 설정이다.
struct RenderFrameSettings {
    static constexpr float      ShadowMapFilterRadiusMin = 1.0f;
    static constexpr float      ShadowMapFilterRadiusMax = 4.0f;
    static constexpr uint32_t   ShadowMapFilterReferenceResolution = 1024;  ///< Filter radius 와 border 를 shadow map 해상도에 맞춰 환산하는 기준 해상도.
    static constexpr uint32_t   ShadowMapFilterReferenceBorderTexels = 6;   ///< 기준 해상도에서 한쪽 shadow map border texel 수 (1.25 * 4 + 1 = 6 texel safety).
    static constexpr float      ShadowMapPoissonKernelMaxRadius = 1.25f;    ///< Shadow map border 계산용. 회전된 Poisson sample 의 최대 정규화 반경 (실제 약 1.234).

    static constexpr uint32_t   CSMCascadeCount = 4;

    uint32_t                    mainRTSampleCount = 1;

    uint32_t                    shadowDebugMode = 0;
    uint32_t                    shadowMapQuality = 2;
    float                       shadowMapFilterRadius = 2.0f;

    uint32_t                    csmResolution = 1024;
    uint32_t                    csmCascadeCount = CSMCascadeCount;
    float                       csmSplitLambda = 0.9f;
    float                       csmMaxDistance = 150.0f;
    float                       csmBlendFraction = 0.1f;
    float                       csmFadeFraction = 0.1f;
    float                       csmConstantDepthBias[CSMCascadeCount] = {1000.0f, 500.0f, 250.0f, 125.0f};
    float                       csmSlopeScaledDepthBias[CSMCascadeCount] = {5.0f, 4.0f, 3.0f, 2.0f};
    
    uint32_t                    lightCullingDebugMode = 0;

    float                       hdrPaperWhiteNits = 200.0f;
    float                       hdrPeakNits = 0.0f;
    float                       meshLODPixelError = 2.0f;

    uint32_t                    taaJitterSampleCount = 16;
    float                       taaHistoryWeight = 0.9f;
    float                       taaColorBoxSigma = 2.0f;
    float                       taaSubpixelWeight = 0.5f;
    float                       taaLuminanceWeight = 0.25f;
    float                       taaDepthFallbackThreshold = 1.0f;

    uint32_t                    hiZDebugPreviewMipLevel = 0;

    float                       aoRadius = 0.5f;

    uint32_t                    ssaoQuality = 1;
    bool                        ssaoDebug = false;
    bool                        ssaoBilateral = false;
    float                       ssaoPower = 2.2f;
    float                       ssaoNormalBiasScale = 0.005f; // Fraction of the AO radius.
    float                       ssaoDepthBiasScale = 0.002f; // Fraction of the AO radius.

    uint32_t                    gtaoQuality = 2;
    uint32_t                    gtaoDenoisePasses = 2;
    bool                        gtao32BitDepth = false;
    uint32_t                    gtaoDebug = 0;
    float                       gtaoPower = 1.0f;
    float                       gtaoFalloffRange = 0.615f;
    float                       gtaoRadiusMultiplier = 1.457f;
    float                       gtaoSampleDistributionPower = 2.0f;
    float                       gtaoThinOccluderCompensation = 0.0f;
    float                       gtaoDepthMipSamplingOffset = 3.3f;

    uint32_t                    ssrMaxIterations = 64;
    uint32_t                    ssrBinaryRefinementSteps = 5;
    float                       ssrMaxDistance = 48.0f;
    float                       ssrThickness = 1.0f;
    float                       ssrNormalBias = 0.02f;

    uint32_t                    toneMappingOperator = 0;
    float                       toneMapLuminanceLogMin = -10.0f;
    float                       toneMapLuminanceLogMax = 2.0f;
    float                       toneMapLuminanceAdaptationRate = 1.1f;
    float                       toneMapMiddleGray = 0.18f;
    float                       toneMapExposureCompensation = 0.0f;
    float                       toneMapMinExposureEV = -8.0f;
    float                       toneMapMaxExposureEV = 2.0f;
    float                       whiteBalanceTemperature = 6500.0f;

    float                       lensDistortion = 0.0f;
    float                       chromaticAberration = 0.0f;
    float                       vignetteIntensity = 0.0f;

    RenderBloomMode             bloomMode = RenderBloomMode::Off;
    float                       bloomThreshold = 1.0f;
    float                       bloomSoftThreshold = 0.5f;
    float                       bloomIntensity = 0.15f;
    float                       bloomDirtIntensity = 0.0f;

    uint32_t                    pyramidBloomMipMaxLevelCount = 1;
    float                       pyramidBloomPrefilterQuality = 1.0f;
    float                       pyramidBloomScatter = 0.67f;
    float                       pyramidBloomPrefilterRadius = 1.25f;
    float                       pyramidBloomDownsampleRadius = 1.0f;
    float                       pyramidBloomUpsampleRadius = 1.0f;

    uint32_t                    convolutionBloomFFTMaxSize = 256;
    float                       convolutionBloomPrefilterRadius = 1.5f;
    float                       convolutionBloomKernelScale = 1.0f;
    float                       convolutionBloomKernelGain = 1.0f;
    float                       convolutionBloomKernelRotation = 0.0f;
    float                       convolutionBloomPaddingTrim = 0.125f;

    bool                        skipViewCulling = false;
    bool                        skipDrawCamera2D = false;
    bool                        skipDrawCamera3D = false;
    bool                        skipPostProcess = false;

    bool                        showDebugLights = false;

    bool                        meshLOD = true;

    bool                        shadows = true;

    bool                        lightCullingZBin = true;
    bool                        depthPrepass = false;
    bool                        specularAA = false;

    bool                        vsync = false;
    bool                        hdrOutput = false;

    bool                        snapshotColorHalfRes = true;
    bool                        taa = true;
    bool                        taaJitter = true;

    bool                        hiZOcclusionCulling = false;
    bool                        hiZOcclusionCullingFreeze = false;
    bool                        hiZDebugPreview = false;

    RenderAOMode                aoMode = RenderAOMode::Off;
    bool                        aoHalfResolution = true;
    bool                        aoTemporalNoise = true;

    bool                        ssr = false;
    bool                        ssrDebug = false;

    bool                        toneMapping = true;
    bool                        colorGrading = false;
    bool                        dithering = true;
};
