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

// 메인 스레드에서 프레임 시작 시 생성되고, 해당 프레임 동안 변경되지 않는 렌더 설정이다.
struct RenderFrameSettings {
    uint32_t                mainRTSampleCount = 1;
    uint32_t                csmResolution = 1024;
    uint32_t                taaJitterSampleCount = 16;
    uint32_t                hiZDebugPreviewMipLevel = 0;
    uint32_t                lightCullingDebugMode = 0;
    uint32_t                toneMappingOperator = 0;
    uint32_t                pyramidBloomMipMaxLevelCount = 1;
    uint32_t                convolutionBloomFFTMaxSize = 256;

    RenderBloomMode         bloomMode = RenderBloomMode::Off;

    float                   hdrPaperWhiteNits = 200.0f;
    float                   hdrPeakNits = 0.0f;

    float                   toneMapLuminanceLogMin = -10.0f;
    float                   toneMapLuminanceLogMax = 2.0f;
    float                   toneMapLuminanceAdaptationRate = 1.1f;
    float                   toneMapMiddleGray = 0.18f;
    float                   toneMapExposureCompensation = 0.0f;
    float                   toneMapMinExposureEV = -8.0f;
    float                   toneMapMaxExposureEV = 2.0f;
    float                   whiteBalanceTemperature = 6500.0f;

    float                   taaHistoryWeight = 0.9f;
    float                   taaColorBoxSigma = 2.0f;
    float                   taaSubpixelWeight = 0.5f;
    float                   taaLuminanceWeight = 0.25f;
    float                   taaDepthFallbackThreshold = 1.0f;

    float                   lensDistortion = 0.0f;
    float                   chromaticAberration = 0.0f;
    float                   vignetteIntensity = 0.0f;

    float                   bloomThreshold = 1.0f;
    float                   bloomSoftThreshold = 0.5f;
    float                   bloomIntensity = 0.15f;
    float                   bloomDirtIntensity = 0.0f;

    float                   pyramidBloomPrefilterQuality = 1.0f;
    float                   pyramidBloomScatter = 0.67f;
    float                   pyramidBloomPrefilterRadius = 1.25f;
    float                   pyramidBloomDownsampleRadius = 1.0f;
    float                   pyramidBloomUpsampleRadius = 1.0f;

    float                   convolutionBloomPrefilterRadius = 1.5f;
    float                   convolutionBloomKernelScale = 1.0f;
    float                   convolutionBloomKernelGain = 1.0f;
    float                   convolutionBloomKernelRotation = 0.0f;
    float                   convolutionBloomPaddingTrim = 0.125f;

    bool                    skipViewCulling = false;
    bool                    skipDrawCamera2D = false;
    bool                    skipDrawCamera3D = false;
    bool                    skipPostProcess = false;

    bool                    showDebugLights = false;
    bool                    lightCullingZBin = true;
    bool                    depthPrepass = false;
    bool                    specularAA = false;

    bool                    vsync = false;
    bool                    hdrOutput = false;

    bool                    snapshotColorHalfRes = true;
    bool                    taa = true;
    bool                    taaJitter = true;

    bool                    hiZOcclusionCulling = false;
    bool                    hiZOcclusionCullingFreeze = false;
    bool                    hiZDebugPreview = false;

    bool                    toneMapping = true;
    bool                    colorGrading = false;
    bool                    dithering = true;
};
