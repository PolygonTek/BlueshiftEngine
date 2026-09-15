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

#include "Core/CVars.h"
#include "RenderFrameSettings.h"

// 메인 스레드에서 현재 CVar 값을 한 프레임의 불변 렌더 설정으로 캡처한다.
RenderFrameSettings CaptureRenderFrameSettings();

extern BE1::CVar    r_skipViewCulling;
extern BE1::CVar    r_skipDrawCamera2D;
extern BE1::CVar    r_skipDrawCamera3D;
extern BE1::CVar    r_skipPostProcess;

extern BE1::CVar    r_debugDrawMaxPrimitives;
extern BE1::CVar    r_debugDrawMaxVertices;
extern BE1::CVar    r_showLights2;
extern BE1::CVar    r_lightCullingZBin;
extern BE1::CVar    r_lightCullingDebug;

extern BE1::CVar    r_vsync;
extern BE1::CVar    r_hdrOutput;
extern BE1::CVar    r_hdrPaperWhiteNits;
extern BE1::CVar    r_hdrPeakNits;

extern BE1::CVar    r_meshLOD;
extern BE1::CVar    r_meshLODPixelError2;

extern BE1::CVar    r_depthPrepass;
extern BE1::CVar    r_specularEnergyCompensation2;
extern BE1::CVar    r_specularAA;

extern BE1::CVar    r_shadows2;
extern BE1::CVar    r_shadowDebug;
extern BE1::CVar    r_shadowMapQuality2;
extern BE1::CVar    r_shadowMapFilterRadius;
extern BE1::CVar    r_csmResolution;
extern BE1::CVar    r_csmCount;
extern BE1::CVar    r_csmSplitLambda;
extern BE1::CVar    r_csmMaxDistance;
extern BE1::CVar    r_csmBlendFraction;
extern BE1::CVar    r_csmFadeFraction;
extern BE1::CVar    r_csmConstantDepthBias0;
extern BE1::CVar    r_csmConstantDepthBias1;
extern BE1::CVar    r_csmConstantDepthBias2;
extern BE1::CVar    r_csmConstantDepthBias3;
extern BE1::CVar    r_csmSlopeScaledDepthBias0;
extern BE1::CVar    r_csmSlopeScaledDepthBias1;
extern BE1::CVar    r_csmSlopeScaledDepthBias2;
extern BE1::CVar    r_csmSlopeScaledDepthBias3;

extern BE1::CVar    r_msaa;
extern BE1::CVar    r_snapshotColorHalfRes;
extern BE1::CVar    r_taa;
extern BE1::CVar    r_taaJitter;
extern BE1::CVar    r_taaJitterSampleCount;
extern BE1::CVar    r_taaHistoryWeight;
extern BE1::CVar    r_taaColorBoxSigma;
extern BE1::CVar    r_taaSubpixelWeight;
extern BE1::CVar    r_taaLuminanceWeight;
extern BE1::CVar    r_taaDepthFallbackThreshold;

extern BE1::CVar    r_hiZOcclusionCulling;
extern BE1::CVar    r_hiZOcclusionCullingFreeze;
extern BE1::CVar    r_hiZDebugPreview;
extern BE1::CVar    r_hiZDebugPreviewMipLevel;

extern BE1::CVar    r_ssao2;
extern BE1::CVar    r_ssaoDebug;
extern BE1::CVar    r_ssaoRadius;
extern BE1::CVar    r_ssaoPower;

extern BE1::CVar    r_ssr;
extern BE1::CVar    r_ssrDebug;
extern BE1::CVar    r_ssrMaxDistance;
extern BE1::CVar    r_ssrThickness;
extern BE1::CVar    r_ssrMaxIterations;
extern BE1::CVar    r_ssrBinaryRefinementSteps;
extern BE1::CVar    r_ssrNormalBias;

extern BE1::CVar    r_textureFilter;
extern BE1::CVar    r_textureMaxAnisotropy;
extern BE1::CVar    r_textureStreamingStreamInDelay;
extern BE1::CVar    r_textureStreamingMaxRequestsPerFrame;
extern BE1::CVar    r_textureStreamingMaxLevelReduction;
extern BE1::CVar    r_textureStreamingMinResidentMaxDimension;

extern BE1::CVar    r_toneMapping;
extern BE1::CVar    r_toneMapOp;
extern BE1::CVar    r_toneMapLuminanceLogMin;
extern BE1::CVar    r_toneMapLuminanceLogMax;
extern BE1::CVar    r_toneMapLuminanceAdaptationRate;
extern BE1::CVar    r_toneMapMiddleGray;
extern BE1::CVar    r_toneMapExposureCompensation;
extern BE1::CVar    r_toneMapMinExposureEV;
extern BE1::CVar    r_toneMapMaxExposureEV;
extern BE1::CVar    r_whiteBalanceTemperature;

extern BE1::CVar    r_colorGrading;
extern BE1::CVar    r_colorGradingSaturation;
extern BE1::CVar    r_colorGradingContrast;
extern BE1::CVar    r_colorGradingGamma;
extern BE1::CVar    r_colorGradingGain;
extern BE1::CVar    r_colorGradingTintR;
extern BE1::CVar    r_colorGradingTintG;
extern BE1::CVar    r_colorGradingTintB;

extern BE1::CVar    r_lensDistortion;
extern BE1::CVar    r_chromaticAberration;
extern BE1::CVar    r_vignette;
extern BE1::CVar    r_dithering;

extern BE1::CVar    r_bloom;
extern BE1::CVar    r_bloomThreshold;
extern BE1::CVar    r_bloomSoftThreshold;
extern BE1::CVar    r_bloomIntensity;
extern BE1::CVar    r_bloomDirtIntensity;

extern BE1::CVar    r_pyramidBloomPrefilterQuality;
extern BE1::CVar    r_pyramidBloomScatter;
extern BE1::CVar    r_pyramidBloomPrefilterRadius;
extern BE1::CVar    r_pyramidBloomDownsampleRadius;
extern BE1::CVar    r_pyramidBloomUpsampleRadius;
extern BE1::CVar    r_pyramidBloomMipCount;

extern BE1::CVar    r_convolutionBloomFFTMaxSize;
extern BE1::CVar    r_convolutionBloomPrefilterRadius;
extern BE1::CVar    r_convolutionBloomKernelScale;
extern BE1::CVar    r_convolutionBloomKernelGain;
extern BE1::CVar    r_convolutionBloomKernelRotation;
extern BE1::CVar    r_convolutionBloomPaddingTrim;
extern BE1::CVar    r_convolutionBloomApertureScale;
extern BE1::CVar    r_convolutionBloomDispersionAmount;
extern BE1::CVar    r_convolutionBloomDebug;
