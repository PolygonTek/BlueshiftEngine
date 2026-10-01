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
    GTAO = 1
};

// 메인 스레드에서 프레임 시작 시 생성되고, 해당 프레임 동안 변경되지 않는 렌더 설정이다.
// 모든 멤버는 사용 전에 CaptureRenderFrameSettings() 에서 설정한다.
struct RenderFrameSettings {
    static constexpr float      ShadowMapFilterRadiusMin = 1.0f;
    static constexpr float      ShadowMapFilterRadiusMax = 4.0f;
    static constexpr uint32_t   ShadowMapFilterReferenceResolution = 1024;  ///< Filter radius 와 border 를 shadow map 해상도에 맞춰 환산하는 기준 해상도.
    static constexpr uint32_t   ShadowMapFilterReferenceBorderTexels = 6;   ///< 기준 해상도에서 한쪽 shadow map border texel 수 (1.25 * 4 + 1 = 6 texel safety).
    static constexpr float      ShadowMapPoissonKernelMaxRadius = 1.25f;    ///< Shadow map border 계산용. 회전된 Poisson sample 의 최대 정규화 반경 (실제 약 1.234).

    static constexpr uint32_t   LocalShadowMinProjectedSize = 32;           ///< Local shadow 후보의 최소 투영 크기 (긴 변). 내부 렌더 해상도 기준 픽셀.
    static constexpr uint32_t   ShadowAtlasMinTileResolution = 64;          ///< Border 를 포함한 local shadow atlas 타일의 최소 해상도.
    static constexpr uint32_t   ShadowAtlasMaxUpdateInterval = 16;          ///< 최대 shadow 갱신 간격 (프레임 수, 2의 거듭제곱).

    static constexpr uint32_t   CSMCascadeCount = 4;

    uint32_t                    mainRTSampleCount;

    uint32_t                    shadowDebugMode;
    uint32_t                    shadowMapQuality;
    float                       shadowMapFilterRadius;

    uint32_t                    shadowAtlasSize;
    uint32_t                    shadowAtlasMaxTileResolution;               ///< Spot 또는 Point 한 면의 최대 타일 해상도. Border 포함.
    uint32_t                    shadowAtlasMaxViews;                        ///< 카메라별 view 예산. Spot 은 1, Point 는 6 을 사용하며 0 이면 비활성화.
    bool                        shadowAtlasLoadBalancing;                   ///< 투영 크기가 작을수록 local shadow 의 갱신 간격을 늘린다.
    float                       shadowProjectedSizeScale;                   ///< Spot/Point 요청 해상도 선택 시 광원 투영 크기에 곱하는 배율.
    uint32_t                    shadowUpdateMinProjectedSize;               ///< 매 프레임 갱신할 광원 투영 영역의 최소 긴 변. 배율 적용 전 내부 렌더 픽셀 기준.

    uint32_t                    csmResolution;
    uint32_t                    csmCascadeCount;
    float                       csmSplitLambda;
    float                       csmMaxDistance;
    float                       csmBlendFraction;
    float                       csmFadeFraction;
    float                       csmConstantDepthBias[CSMCascadeCount];
    float                       csmSlopeScaledDepthBias[CSMCascadeCount];
    
    uint32_t                    lightCullingDebugMode;

    float                       hdrPaperWhiteNits;
    float                       hdrPeakNits;
    float                       meshLODPixelError;

    uint32_t                    taaJitterSampleCount;
    float                       taaHistoryWeight;
    float                       taaColorBoxSigma;
    float                       taaSubpixelWeight;
    float                       taaLuminanceWeight;
    float                       taaDepthFallbackThreshold;

    uint32_t                    hiZDebugPreviewMipLevel;

    float                       aoRadius;

    uint32_t                    gtaoQuality;
    uint32_t                    gtaoDenoisePasses;
    bool                        gtao32BitDepth;
    uint32_t                    gtaoDebug;
    float                       gtaoPower;
    float                       gtaoFalloffRange;
    float                       gtaoRadiusMultiplier;
    float                       gtaoSampleDistributionPower;
    float                       gtaoThinOccluderCompensation;
    float                       gtaoDepthMipSamplingOffset;

    uint32_t                    ssrMaxIterations;
    uint32_t                    ssrBinaryRefinementSteps;
    float                       ssrMaxDistance;
    float                       ssrThickness;
    float                       ssrNormalBias;

    uint32_t                    toneMappingOperator;
    float                       toneMapLuminanceLogMin;
    float                       toneMapLuminanceLogMax;
    float                       toneMapLuminanceAdaptationRate;
    float                       toneMapMiddleGray;
    float                       toneMapExposureCompensation;
    float                       toneMapMinExposureEV;
    float                       toneMapMaxExposureEV;
    float                       whiteBalanceTemperature;

    float                       lensDistortion;
    float                       chromaticAberration;
    float                       vignetteIntensity;

    RenderBloomMode             bloomMode;
    float                       bloomThreshold;
    float                       bloomSoftThreshold;
    float                       bloomIntensity;
    float                       bloomDirtIntensity;

    uint32_t                    pyramidBloomMipMaxLevelCount;
    float                       pyramidBloomPrefilterQuality;
    float                       pyramidBloomScatter;
    float                       pyramidBloomPrefilterRadius;
    float                       pyramidBloomDownsampleRadius;
    float                       pyramidBloomUpsampleRadius;

    uint32_t                    convolutionBloomFFTMaxSize;
    float                       convolutionBloomPrefilterRadius;
    float                       convolutionBloomKernelScale;
    float                       convolutionBloomKernelGain;
    float                       convolutionBloomKernelRotation;
    float                       convolutionBloomPaddingTrim;

    bool                        skipViewCulling;
    bool                        skipDrawCamera2D;
    bool                        skipDrawCamera3D;
    bool                        skipPostProcess;

    bool                        showDebugLights;

    bool                        meshLOD;

    bool                        shadows;

    bool                        lightCullingZBin;
    bool                        depthPrepass;
    bool                        specularAA;

    bool                        vsync;
    bool                        hdrOutput;

    bool                        snapshotColorHalfRes;
    bool                        taa;
    bool                        taaJitter;

    bool                        hiZOcclusionCulling;
    bool                        hiZOcclusionCullingFreeze;
    bool                        hiZDebugPreview;

    RenderAOMode                aoMode;
    bool                        aoHalfResolution;
    bool                        aoTemporalNoise;

    bool                        ssr;
    bool                        ssrDebug;

    bool                        toneMapping;
    bool                        colorGrading;
    bool                        dithering;
};
