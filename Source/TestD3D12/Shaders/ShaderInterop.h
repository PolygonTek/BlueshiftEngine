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

#ifndef __SHADERINTEROP_H__
#define __SHADERINTEROP_H__

#ifdef __cplusplus

#include <cstddef>

#include "Shaders/ShaderTypes.h"

#define column_major
#define row_major

#else

#include "ShaderPacking.hlsli"

#endif // __cplusplus

static const uint TextureStreamingFeedbackResolution = 16384;
static const uint TextureStreamingFeedbackMaxLOD = 14;
static const uint TextureStreamingInvalidIndex = 0xFFFFFFFF;

static const uint GenMip2DBlockSizeX = 32;
static const uint GenMip2DBlockSizeY = 32;
static const uint GenMip3DBlockSizeX = 8;
static const uint GenMip3DBlockSizeY = 8;
static const uint GenMip3DBlockSizeZ = 8;
static const uint MipGenOptionBitPreserveCoverage = 1 << 0;
static const uint MipGenOptionBitSRGB = 1 << 1;

static const uint IBLIntegrationBlockSizeX = 8;
static const uint IBLIntegrationBlockSizeY = 8;

static const uint MSAAResolveDepthBlockSizeX = 16;
static const uint MSAAResolveDepthBlockSizeY = 16;
static const uint MSAAResolveVelocityBlockSizeX = 16;
static const uint MSAAResolveVelocityBlockSizeY = 16;

static const uint LuminanceBlockSize = 32;
static const uint LuminanceHistogramBinCount = LuminanceBlockSize * LuminanceBlockSize;
static const uint LuminanceBufferOffsetExposure = 0;
static const uint LuminanceBufferOffsetLuminance = LuminanceBufferOffsetExposure + 4;
static const uint LuminanceBufferOffsetHistogram = LuminanceBufferOffsetLuminance + 4;

static const uint FinalPostProcessBlockSizeX = 16;
static const uint FinalPostProcessBlockSizeY = 16;
static const uint ColorGradingLUTSize = 32;

static const uint FinalPostProcessFlagToneMapping = 1u << 0;
static const uint FinalPostProcessFlagDithering = 1u << 1;
static const uint FinalPostProcessFlagColorGrading = 1u << 2;
static const uint FinalPostProcessFlagHDROutput = 1u << 3;

static const uint TAAResolveBlockSizeX = 16;
static const uint TAAResolveBlockSizeY = 16;

static const uint ToneMappingOperatorLinear = 0;
static const uint ToneMappingOperatorExponential = 1;
static const uint ToneMappingOperatorLogarithmic = 2;
static const uint ToneMappingOperatorDragoLogarithmic = 3;
static const uint ToneMappingOperatorReinhard = 4;
static const uint ToneMappingOperatorReinhardExtended = 5;
static const uint ToneMappingOperatorFilmicALU = 6;
static const uint ToneMappingOperatorFilmicACES = 7;
static const uint ToneMappingOperatorFilmicUnreal3 = 8;
static const uint ToneMappingOperatorFilmicUncharted2 = 9;
static const uint ToneMappingOperatorFilmicGT = 10;
static const uint ToneMappingOperatorPBRNeutral = 11;

static const uint BloomBlockSizeX = 16;
static const uint BloomBlockSizeY = 16;

static const uint ConvolutionBloomApertureEnergyReduceBlockSize = 256;

static const uint GenHiZMipBlockSizeX = 8;
static const uint GenHiZMipBlockSizeY = 8;
static const uint HiZDebugPreviewBlockSizeX = 16;
static const uint HiZDebugPreviewBlockSizeY = 16;

static const uint AOUpsampleBlockSizeX = 8;
static const uint AOUpsampleBlockSizeY = 8;

static const uint ScreenSpaceDepthPyramidBlockSizeX = 8;
static const uint ScreenSpaceDepthPyramidBlockSizeY = 8;

static const uint SSRTraceBlockSizeX = 8;
static const uint SSRTraceBlockSizeY = 8;

static const uint GTAODepthMipLevelCount = 5;
static const uint GTAOPrefilterBlockSizeX = 8;
static const uint GTAOPrefilterBlockSizeY = 8;
static const uint GTAOMainBlockSizeX = 8;
static const uint GTAOMainBlockSizeY = 8;
static const uint GTAODenoiseBlockSizeX = 8;
static const uint GTAODenoiseBlockSizeY = 8;

static const uint GTAOQualityLevelCount = 4;
static const uint GTAOQualityLowSliceCount = 1;
static const uint GTAOQualityLowStepsPerSlice = 3;
static const uint GTAOQualityMediumSliceCount = 2;
static const uint GTAOQualityMediumStepsPerSlice = 3;
static const uint GTAOQualityHighSliceCount = 3;
static const uint GTAOQualityHighStepsPerSlice = 3;
static const uint GTAOQualityUltraSliceCount = 9;
static const uint GTAOQualityUltraStepsPerSlice = 3;

static const uint ScreenSpaceRayTraceStatusMiss = 0;
static const uint ScreenSpaceRayTraceStatusHit = 1;
static const uint ScreenSpaceRayTraceStatusMaxIterations = 2;

static const uint HiZCullThreadCount = 64;
static const uint HiZCullIndirectArgInstanceCountOffset = 4;
static const uint HiZCullIndirectArgSize = 20;

static const uint ShaderSurfaceFlagNonUniformScale = 1u << 0;
static const uint ShaderSkinningJointOffsetInvalid = 0xFFFFFFFF;

// Independent material properties shared by CPU and GPU code.
static const uint ShaderMaterialFlagTransparent = 1u << 0;

static const uint ShaderLightTypeBitCount = 2;
static const uint ShaderLightTypeMask = (1u << ShaderLightTypeBitCount) - 1u;
static const uint ShaderLightTypePoint = 0;
static const uint ShaderLightTypeSpot = 1;
static const uint ShaderLightTileSize = 32;
static const uint ShaderLightTileCoordBitCount = 16;
static const uint ShaderLightTileCoordMask = (1u << ShaderLightTileCoordBitCount) - 1u;
static const uint ShaderLightWordBits = 32;
static const uint ShaderLightWordShift = 5;
static const uint ShaderLightCullingThreadGroupSizeX = 8;
static const uint ShaderLightCullingThreadGroupSizeY = 8;
static const uint ShaderLightZBinCount = 4096;
static const uint ShaderLightZBinMaxLightCount = 1u << 16;
static const uint ShaderLightZBinIndexMask = ShaderLightZBinMaxLightCount - 1u;
static const uint ShaderLightZBinEmpty = ShaderLightZBinIndexMask; // min ID = 0xFFFF, max ID = 0
static const uint ShaderLightInvalidOffset = 0xFFFFFFFFu;
static const uint ShaderLightCullingDebugModeNone = 0;
static const uint ShaderLightCullingDebugModeTileMask = 1;
static const uint ShaderLightCullingDebugModeTileMaskAndZBin = 2;

static const uint ShaderShadowDebugModeNone = 0;
static const uint ShaderShadowDebugModeVisibility = 1;
static const uint ShaderShadowDebugModeCascadeCoverage = 2;
static const uint ShaderCSMCascadeMaxCount = 4;

static const uint ShaderLocalShadowIndexInvalid = 0xFFFFFFFFu;

// Point shadow 는 광원 회전과 무관한 월드 축 6면을 사용한다. View 는 firstShadowView + faceIndex 로 찾는다.
static const uint ShaderPointShadowFacePositiveX = 0;
static const uint ShaderPointShadowFaceNegativeX = 1;
static const uint ShaderPointShadowFacePositiveY = 2;
static const uint ShaderPointShadowFaceNegativeY = 3;
static const uint ShaderPointShadowFacePositiveZ = 4;
static const uint ShaderPointShadowFaceNegativeZ = 5;
static const uint ShaderPointShadowFaceInvalid = 0xFFFFFFFFu;

static const uint ShaderCameraFlagProjectionOrthogonal = 1u << 0;
static const uint ShaderCameraFlagUseLightZBin = 1u << 1;
static const uint ShaderCameraFlagUseDirectionalLight = 1u << 2;
static const uint ShaderCameraFlagUseSpecularAA = 1u << 3;
static const uint ShaderCameraFlagUseAO = 1u << 4;

static const uint MultiplySpectrum2DBlockSize = 256;

static const uint OceanInitialSpectrumBlockSizeX = 8;
static const uint OceanInitialSpectrumBlockSizeY = 8;
static const uint OceanTimeSpectrumBlockSizeX = 8;
static const uint OceanTimeSpectrumBlockSizeY = 8;
static const uint OceanResolveFieldsBlockSizeX = 8;
static const uint OceanResolveFieldsBlockSizeY = 8;
static const uint OceanSurfaceQueryBlockSize = 64;
static const uint OceanSurfaceQueryFixedPointIterationCount = 3;

static const uint OceanSpectrumComplexPlaneCount = 4;
static const uint OceanCascadeCount = 3;
static const uint OceanTimeSpectrumOptionBitCrossDerivative = 1u << 0;
static const uint OceanFoamOptionBitEnabled = 1u << 0;
static const uint OceanFoamOptionBitResetHistory = 1u << 1;
static const uint OceanSurfaceOptionBitFoamEnabled = 1u << 0;
static const uint OceanSurfaceOptionBitRefractionEnabled = 1u << 1;

#ifdef __cplusplus
static_assert(OceanCascadeCount >= 2 && OceanCascadeCount <= 4, "GPU ocean supports between two and four cascades");
#endif

static const uint FFTRadix2_256StageCount = 8;
static const uint FFTRadix2_256ButterflyCount = 256 / 2;
static const uint FFTRadix2_256BlockSize = FFTRadix2_256ButterflyCount;
static const uint FFTRadix4_256StageCount = 4;
static const uint FFTRadix4_256ButterflyCount = 256 / 4;
static const uint FFTRadix4_256BlockSize = FFTRadix4_256ButterflyCount;
static const uint FFTRadix8_512StageCount = 3;
static const uint FFTRadix8_512ButterflyCount = 512 / 8;
static const uint FFTRadix8_512BlockSize = FFTRadix8_512ButterflyCount;
static const uint FFTRadix8_1024StageCount = 3;
static const uint FFTRadix8_1024ButterflyCount = 1024 / 8;
static const uint FFTRadix8_1024BlockSize = FFTRadix8_1024ButterflyCount;
static const uint FFTRadix2_1024ButterflyCount = 1024 / 2;
static const uint FFTRadix2_1024BlockSize = FFTRadix2_1024ButterflyCount;

static const uint TransposeComplexTileSize = 16;

struct MipGenParams {
    uint3 dstSize;
    uint firstSlice;
    float3 dstSizeRcp;
    uint flags;
};

struct GenLDSumGGXParams {
    uint radianceCubeMapSize;
    float roughness;
    float minRadianceMipLevel;
};

#ifdef __cplusplus
static_assert(sizeof(GenLDSumGGXParams) == 12, "GenLDSumGGXParams must match the HLSL root constants");
#endif

struct GenLDSumPhongSpecularParams {
    float specularPower;
};

#ifdef __cplusplus
static_assert(sizeof(GenLDSumPhongSpecularParams) == 4, "GenLDSumPhongSpecularParams must match the HLSL root constants");
#endif

struct ReconstructCubemapSHParams {
    float3 bandFactors;
};

#ifdef __cplusplus
static_assert(sizeof(ReconstructCubemapSHParams) == 12, "ReconstructCubemapSHParams must match the HLSL root constants");
#endif

struct LuminancePass1Params {
    uint2 sampleSize;
    float2 sampleSizeRcp;
    float luminanceLogMin;
    float luminanceLogRangeRcp;
};

struct LuminancePass2Params {
    float luminanceLogMin;
    float luminanceLogRange;
    float luminancePixelCount;
    float luminanceAdaptationRate;
    float middleGray;
    float exposureCompensation;
    float minExposure;
    float maxExposure;
};

struct FinalPostProcessParams {
    uint2 outputSize;
    float2 outputTexelSize;
    uint flags;
    uint toneMappingOperator;
    float bloomIntensity;
    float chromaticAberration;
    float vignetteIntensity;
    float lensDistortion;
    float lensDistortionFitScale;
    float lensDistortionRadiusSqScale;
    float3 whiteBalance;
    float bloomDirtIntensity;
    float2 bloomDirtUvScale;
    float paperWhiteNits;           // Absolute luminance represented by linear 1.0.
    float maxDisplayLinearValue;    // Peak luminance in paper-white-relative linear units.
};

#ifdef __cplusplus
static_assert(sizeof(FinalPostProcessParams) == 80, "FinalPostProcessParams must match the HLSL constant-buffer layout");
#endif

struct TAAResolveParams {
    uint2 outputSize;
    float2 outputTexelSize;
    float historyWeight;
    uint historyValid;
    uint useMinDepth;
    float depthFallbackThreshold;
    float maxSubpixelCurrentWeight;
    float luminanceWeight;
    float colorBoxSigma;
    uint padding0;
};

#ifdef __cplusplus
static_assert(sizeof(TAAResolveParams) == 48, "TAAResolveParams must match the HLSL constant-buffer layout");
#endif

struct MSAAResolveDepthParams {
    uint2 outputSize;
    uint useMinDepth;
    uint padding0;
};

struct MSAAResolveVelocityParams {
    uint2 outputSize;
    uint useMinDepth;
    uint padding0;
};

struct FullscreenImageParams {
    float2 texCoordScale;
    float2 texCoordOffset;
};

struct HDR10OutputParams {
    float2 texCoordScale;
    float2 texCoordOffset;
    float paperWhiteNits;
    float maxDisplayLinearValue;
};

struct DisplayP3EDROutputParams {
    float2 texCoordScale;
    float2 texCoordOffset;
    float maxDisplayLinearValue;
    float padding0;
};

#ifdef __cplusplus
static_assert(sizeof(HDR10OutputParams) == 24, "HDR10OutputParams must match the HLSL root constants");
static_assert(sizeof(DisplayP3EDROutputParams) == 24, "DisplayP3EDROutputParams must match the HLSL root constants");
#endif

struct BloomParams {
    uint2 inputSize;
    uint2 outputSize;
    float2 inputTexelSize;
    float2 outputTexelSize;
    float4 params0;
    float4 params1;
};

#define BP_BLOOM_THRESHOLD bloomParams.params0.x
#define BP_BLOOM_SOFT_THRESHOLD bloomParams.params0.y
#define BP_BLOOM_PREFILTER_QUALITY bloomParams.params0.z
#define BP_BLOOM_WEIGHT bloomParams.params0.z
#define BP_BLOOM_RADIUS bloomParams.params0.w
#define BP_BLOOM_CONTENT_OFFSET bloomParams.params1.xy
#define BP_BLOOM_CONTENT_SIZE bloomParams.params1.zw
#define BP_BLOOM_SCREEN_MIP_LEVEL bloomParams.params0.z
#define BP_BLOOM_KERNEL_SCALE bloomParams.params0.x
#define BP_BLOOM_KERNEL_ROTATION_COS bloomParams.params0.x
#define BP_BLOOM_KERNEL_ROTATION_SIN bloomParams.params0.y
#define BP_BLOOM_MAPPED_KERNEL_SCALE bloomParams.params0.zw
#define BP_BLOOM_KERNEL_MIP_LEVEL bloomParams.params1.x
#define BP_BLOOM_APERTURE_SCALE bloomParams.params0.x
#define BP_BLOOM_APERTURE_MIP_LEVEL bloomParams.params0.y
#define BP_BLOOM_DISPERSION_AMOUNT bloomParams.params0.x
#define BP_BLOOM_DISPERSION_MIN_WAVELENGTH_NM bloomParams.params0.y
#define BP_BLOOM_DISPERSION_MAX_WAVELENGTH_NM bloomParams.params0.z
#define BP_BLOOM_DISPERSION_REFERENCE_WAVELENGTH_NM bloomParams.params0.w
#define BP_BLOOM_DISPERSION_SAMPLE_COUNT bloomParams.params1.x
#define BP_BLOOM_DISPERSION_INTENSITY_SCALE bloomParams.params1.y

struct HiZMipGenParams {
    uint2 inputSize;
    float2 inputSizeRcp;
    uint2 outputSize;
    uint outputMipCount;
    uint useMinDepth;
};

struct GTAOPrefilterParams {
    uint2 inputSize;
    uint2 outputSize;
    uint resolutionScale;
    uint useMaxDepth;
    uint depthMipCount;
    uint padding0;
    float radius;
    float falloffRange;
    float radiusMultiplier;
    float farPlaneDeviceDepth;
};

#ifdef __cplusplus
static_assert(sizeof(GTAOPrefilterParams) == 48, "GTAOPrefilterParams must match the HLSL root constants");
#endif

struct GTAOMainParams {
    uint2 inputSize;
    uint2 outputSize;
    float2 outputSizeRcp;
    uint resolutionScale;
    uint noiseIndex;
    uint depthMipCount;
    float radius;
    float falloffRange;
    float radiusMultiplier;
    float sampleDistributionPower;
    float thinOccluderCompensation;
    float depthMipSamplingOffset;
    float power;
    float farPlaneDeviceDepth;
    float padding0;
};

#ifdef __cplusplus
static_assert(sizeof(GTAOMainParams) == 72, "GTAOMainParams must match the HLSL root constants");
#endif

struct GTAODenoiseParams {
    uint2 outputSize;
    float2 outputSizeRcp;
    float blurBeta;
    uint finalPass;
    float2 padding0;
};

#ifdef __cplusplus
static_assert(sizeof(GTAODenoiseParams) == 32, "GTAODenoiseParams must match the HLSL root constants");
#endif

struct GTAODebugParams {
    uint2 inputSize;
    uint2 outputSize;
    uint sourceType;
    uint mipLevel;
    float linearDepthScale;
    float padding0;
};

#ifdef __cplusplus
static_assert(sizeof(GTAODebugParams) == 32, "GTAODebugParams must match the HLSL root constants");
#endif

struct AOUpsampleParams {
    uint2 inputSize;
    uint2 outputSize;
    float depthSigmaRcp;
    float farPlaneDeviceDepth;
    float2 padding0;
};

#ifdef __cplusplus
static_assert(sizeof(AOUpsampleParams) == 32, "AOUpsampleParams must match the HLSL root constants");
#endif

struct ScreenSpaceDepthPyramidParams {
    uint2 inputSize;
    uint2 outputSize;
    uint2 validOutputSize;
    float farPlaneDeviceDepth;
    uint useMinDepth;
};

#ifdef __cplusplus
static_assert(sizeof(ScreenSpaceDepthPyramidParams) == 32, "ScreenSpaceDepthPyramidParams must match the HLSL root constants");
#endif

struct SSRTraceParams {
    uint2 inputSize;
    uint2 outputSize;

    uint depthPyramidMipCount;
    uint maxIterations;
    uint binaryRefinementSteps;
    uint useMinDepth;

    float maxDistance;
    float thickness;
    float normalBias;
};

#ifdef __cplusplus
static_assert(sizeof(SSRTraceParams) == 44, "SSRTraceParams must match the HLSL root constants");
#endif

struct HiZDebugPreviewParams {
    uint2 inputSize;
    uint2 outputSize;
    float linearDepthScale;
    uint3 padding0;
};

#ifdef __cplusplus
static_assert(sizeof(HiZDebugPreviewParams) == 32, "HiZDebugPreviewParams must match the HLSL constant-buffer layout");
#endif

struct HiZCullInstancesParams {
    uint inputInstanceCount;
    uint surfaceTableIndex;
    uint hiZWidth;
    uint hiZHeight;
    uint hiZMipCount;
    uint useMinDepth;
    uint instanceBaseOffset;
    uint indirectArgOffset;
};

struct MultiplySpectrum2DParams {
    uint fftSize;
    uint planeCount;
    float scale;
    uint padding0;
};

struct FFTStageParams {
    float sign;
    float scale;
    uint padding0;
    uint padding1;
};

struct OceanInitialSpectrumCascadeParams {
    uint randomSeed;
    float patchSize;
    float minWaveNumber;
    float maxWaveNumber;
};

#ifdef __cplusplus
static_assert(sizeof(OceanInitialSpectrumCascadeParams) == 16, "OceanInitialSpectrumCascadeParams must match the HLSL constant-buffer layout");
#endif

struct OceanPhillipsInitialSpectrumParams {
    float spectrumScale;
    float windSpeed;
    float2 windDirection;
    float gravity;
    float smallWaveDamping;
    float oppositeWaveDamping;
    float padding0;
    OceanInitialSpectrumCascadeParams cascadeParams[OceanCascadeCount];
};

#ifdef __cplusplus
static_assert(sizeof(OceanPhillipsInitialSpectrumParams) == 32 + sizeof(OceanInitialSpectrumCascadeParams) * OceanCascadeCount,
    "OceanPhillipsInitialSpectrumParams must match the HLSL constant-buffer layout");
#endif

struct OceanTMAInitialSpectrumParams {
    float spectrumScale;
    float windSpeed;
    float2 windDirection;
    float gravity;
    float smallWaveDamping;
    float alpha;
    float peakOmega;
    float depth;
    float swell;
    float spread;
    float gamma;
    OceanInitialSpectrumCascadeParams cascadeParams[OceanCascadeCount];
};

#ifdef __cplusplus
static_assert(sizeof(OceanTMAInitialSpectrumParams) == 48 + sizeof(OceanInitialSpectrumCascadeParams) * OceanCascadeCount,
    "OceanTMAInitialSpectrumParams must match the HLSL constant-buffer layout");
#endif

struct OceanTimeSpectrumParams {
    float time;
    float gravity;
    float depth;
    float choppiness;
    float patchSize;
    uint cascadeIndex;
    uint options;
    float padding0;
};

#ifdef __cplusplus
static_assert(sizeof(OceanTimeSpectrumParams) == 32, "OceanTimeSpectrumParams must match the HLSL constant-buffer layout");
#endif

struct OceanResolveFieldsParams {
    float deltaTime;
    float displacementScale;
    float foamThreshold;
    float foamGenerationRate;
    float foamDecayRate;
    uint foamOptions;
    uint cascadeIndex;
    float padding0;
};

#ifdef __cplusplus
static_assert(sizeof(OceanResolveFieldsParams) == 32, "OceanResolveFieldsParams must match the HLSL constant-buffer layout");
#endif

struct OceanSurfaceQueryInput {
    float2 worldPositionXY;     ///< horizontal world position at which the displaced surface is requested
    float sampleDiameter;       ///< world-space filter diameter in meters; zero samples mip 0
    uint queryId;               ///< result 에 그대로 전달되는 caller-owned opaque ID
};

#ifdef __cplusplus
static_assert(sizeof(OceanSurfaceQueryInput) == 16, "OceanSurfaceQueryInput must match the HLSL structured-buffer layout");
#endif

struct OceanSurfaceQueryResult {
    float3 surfacePosition;
    uint queryId;
    float3 surfaceNormal;
    float sampleTime;           ///< ocean simulation time represented by this result
};

#ifdef __cplusplus
static_assert(sizeof(OceanSurfaceQueryResult) == 32, "OceanSurfaceQueryResult must match the HLSL structured-buffer layout");
#endif

struct OceanSurfaceQueryParams {
    float4 cascadePatchSizes;
    float surfaceHeight;
    float displacementScale;
    float sampleTime;
    uint queryCount;
};

#ifdef __cplusplus
static_assert(sizeof(OceanSurfaceQueryParams) == 32, "OceanSurfaceQueryParams must match the HLSL constant-buffer layout");
#endif

struct OceanSurfaceParams {
    float4 cascadePatchSizes;
    float4 cascadeVertexMipLevels;
    float surfaceHeight;
    float roughness;
    float displacementScale;
    uint surfaceOptions;
    float3 waterColor;
    float fresnelF0;
    float3 scatterColor;
    float scatterHeight;
    float3 foamColor;
    float cascadeFadeScale;
    float foamDetailMapSize;
    float foamDetailStrength;
    float refractionDistance;
    float absorptionDistance;
    float airToWaterEta;
    float waterToAirEta;
    float2 sceneSize;
    float2 sceneSizeRcp;
    float2 sceneSnapshotSize;
    float2 sceneSnapshotSizeRcp;
    float2 padding;
};

#ifdef __cplusplus
static_assert(sizeof(OceanSurfaceParams) == 160, "OceanSurfaceParams must match the HLSL constant-buffer layout");
#endif

struct ShaderLightCullingParams {
    uint numLocalLights;
    uint lightRefTableOffset;
    uint tileWordTableOffset;
    uint tileCountX;
    uint tileCountY;
    uint wordsPerTile;
    uint padding0;
    uint padding1;
};

#ifdef __cplusplus
static_assert(sizeof(ShaderLightCullingParams) == 32, "ShaderLightCullingParams must match the HLSL constant-buffer layout");
#endif

struct ShaderDirectionalLight {
    float3 direction;
    uint flags;
    float3 intensityRGB; // Luminance-normalized RGB times normal illuminance (lux).
    float padding0;
};

#ifdef __cplusplus
static_assert(sizeof(ShaderDirectionalLight) == 32, "ShaderDirectionalLight must match the HLSL constant-buffer layout");
#endif

struct ShaderPointLight {
    float4 worldToProjectionU;
    float4 worldToProjectionV;
    float3 origin;
    float invRadiusSquared;
    float3 intensityRGB; // Luminance-normalized RGB times luminous intensity (cd).
    uint shadowIndex; // 카메라의 local shadow 테이블 인덱스. ShaderLocalShadowIndexInvalid 이면 shadow 없음.
};

#ifdef __cplusplus
static_assert(sizeof(ShaderPointLight) == 64, "ShaderPointLight must remain a tightly packed StructuredBuffer element");
static_assert(offsetof(ShaderPointLight, shadowIndex) == 60, "Point shadow index must match the HLSL structured-buffer layout");
#endif

struct ShaderSpotLight {
    float3 origin;
    float invRadiusSquared;
    float3 direction; // 광원에서 나가는 단위 방향.
    float cosOuterCone;
    float3 intensityRGB; // Luminance-normalized RGB times luminous intensity inside the inner cone (cd).
    float invCosConeDifference;
    uint shadowIndex; // 카메라의 local shadow 테이블 인덱스. ShaderLocalShadowIndexInvalid 이면 shadow 없음.
};

#ifdef __cplusplus
static_assert(sizeof(ShaderSpotLight) == 52, "ShaderSpotLight must remain a tightly packed StructuredBuffer element");
static_assert(offsetof(ShaderSpotLight, direction) == 16, "Spot direction must match the HLSL structured-buffer layout");
static_assert(offsetof(ShaderSpotLight, shadowIndex) == 48, "Spot shadow index must match the HLSL structured-buffer layout");
#endif

struct FrameConstants {
    float frameTime;
    float elapsedTime;
    float elapsedPrevTime;
    uint frameCount;
    uint shaderMaterialTableIndex;
    uint shaderPointLightTableIndex;
    uint shaderSpotLightTableIndex;
    uint shaderLightRefTableIndex;
    uint shaderLightZBinTableIndex;
    uint shaderLightTileWordTableIndex;
    uint feedbackBufferIndex;
    uint dfgSumGGXTextureIndex;
    uint randomRotMatTextureIndex;
    uint padding0;
    uint padding1;
    uint padding2;
    ShaderDirectionalLight directionalLight;
};

#ifdef __cplusplus
static_assert(sizeof(FrameConstants) == 96, "FrameConstants must match the HLSL constant-buffer layout");
static_assert(offsetof(FrameConstants, directionalLight) == 64, "Directional light must start at a constant-buffer register boundary");
#endif

// Shadow depth 기록에 필요한 광원 행렬과 surface table. 샘플링용 데이터와 별도로 전달한다.
struct ShadowViewConstants {
    row_major float4x4 viewProjMatrix;
    uint surfaceTableIndex;
    uint3 padding;
};

#ifdef __cplusplus
static_assert(sizeof(ShadowViewConstants) == 80, "ShadowViewConstants must match the HLSL constant-buffer layout");
#endif

// 광원 하나의 샘플링 정보. 광원 타입은 ShaderPointLight/ShaderSpotLight 에서 이미 결정된다.
// 인덱스는 현재 카메라의 GPU 테이블 기준이며 CPU allocator handle 이나 영속 광원 ID 가 아니다.
struct ShaderLocalShadow {
    uint firstShadowView;       // View 테이블에서 이 광원의 첫 요소. Point 6면도 연속된 요소를 사용한다.
    uint numShadowViews;        // Spot: 1, Point: 6. 0 이면 유효한 view 가 없다.
    float filterRadius;         // Atlas texel 단위의 PCF 반경. 업로드 시 border 안에 들어가도록 제한한다.
    float normalizedBorder;     // borderTexels / 할당된 타일 해상도. Point 는 모든 면이 같은 해상도/border 를 사용한다.
    float3 pointOrigin;         // Point depth 기록 당시의 광원 위치. 조명 계산의 현재 위치와 별개다.
    float2 pointDepthScaleBias; // Point 면의 전방 거리 d 로 depth = X + Y / d 를 계산한다.
};

// Spot 한 개 또는 Point 한 면의 샘플링 정보. 렌더링용 ShadowViewConstants 와 별도다.
struct ShaderLocalShadowView {
    // world position -> homogeneous tile coordinate. XYZ 를 W 로 나눈 뒤 XY 는 타일 전체의 UV (Y down), Z 는 비교 depth [0, 1].
    // border 를 포함한 reverse-Z 렌더링 투영을 사용하며 near = 1, far = 0 이다. perspective divide 전 W <= 0 은 샘플링하지 않는다.
    // 포인트 라이트는 보존한 위치와 depth 계수로 좌표를 직접 계산하므로 이 행렬을 사용하지 않는다.
    row_major float4x4 worldToShadowCoordMatrix;
    float4 atlasScaleBias;      // XY = tileSize / atlasSize, ZW = tileOrigin / atlasSize. atlasUV = tileUV * XY + ZW.
};

// 현재 카메라의 atlas 와 두 StructuredBuffer 를 참조하는 샘플링 데이터.
struct ShaderShadowAtlasData {
    uint textureIndex;          // Texture2D<float> 의 bindless SRV index.
    uint shadowTableIndex;      // StructuredBuffer<ShaderLocalShadow> 의 bindless SRV index.
    uint shadowViewTableIndex;  // StructuredBuffer<ShaderLocalShadowView> 의 bindless SRV index.
    uint numShadows;            // 0 이면 atlas/테이블에 접근하지 않는다. 업로드와 depth 기록을 마친 뒤에만 활성화한다.
    float2 texelSize;           // 1 / atlas 크기. 모든 타일의 PCF offset 에 공유한다.
    uint numShadowViews;
    uint padding;
};

#ifdef __cplusplus
static_assert(sizeof(ShaderLocalShadow) == 36, "ShaderLocalShadow must match the HLSL structured-buffer stride");
static_assert(offsetof(ShaderLocalShadow, numShadowViews) == 4 && offsetof(ShaderLocalShadow, filterRadius) == 8 && offsetof(ShaderLocalShadow, normalizedBorder) == 12, "Local shadow field offsets must match HLSL");
static_assert(offsetof(ShaderLocalShadow, pointOrigin) == 16 && offsetof(ShaderLocalShadow, pointDepthScaleBias) == 28, "Point shadow sampling offsets must match HLSL");
static_assert(sizeof(ShaderLocalShadowView) == 80, "ShaderLocalShadowView must match the HLSL structured-buffer stride");
static_assert(offsetof(ShaderLocalShadowView, atlasScaleBias) == 64, "Local shadow view field offsets must match HLSL");
static_assert(sizeof(ShaderShadowAtlasData) == 32, "ShaderShadowAtlasData must match the HLSL constant-buffer layout");
static_assert(offsetof(ShaderShadowAtlasData, numShadows) == 12 && offsetof(ShaderShadowAtlasData, texelSize) == 16 && offsetof(ShaderShadowAtlasData, numShadowViews) == 24 && offsetof(ShaderShadowAtlasData, padding) == 28, "Shadow atlas field offsets must match HLSL");
#endif

// CSM 을 샘플링하는 데 필요한 데이터.
struct ShaderCSMData {
    // world position -> shadow texture coordinate.
    // XY 는 UV, Z 는 비교할 depth [0, 1].
    // orthogonal projection 이므로 perspective divide 없이 float3x4 로 표현할 수 있다.
    row_major float3x4 worldToShadowCoordMatrices[ShaderCSMCascadeMaxCount];

    uint textureIndex;          // Texture2DArray<float> 의 bindless SRV index.
    uint cascadeCount;
    uint enabled;               // 0 이면 shadow texture 에 접근하지 않는다.
    float maxDistance;          // 최대 shadow 적용 거리.
    float4 cascadeFilterRadii;  // 현재 해상도와 cascade 크기에 맞춘 texel 반경.
    float2 texelSize;
    float normalizedBorder;     // normalized border width in [0, 1] range.
    float invBlendWidth;        // normalized inverse blend width.
    float farFadeStart;
    float invFarFadeRange;
    uint2 padding;
};

#ifdef __cplusplus
static_assert(sizeof(ShaderCSMData) == 256, "CSM data must match the HLSL constant-buffer layout");
#endif

struct CameraConstants {
    row_major float4x4 viewProjMatrix;
    row_major float4x4 invViewProjMatrix;
    row_major float4x4 unjitteredViewProjMatrix;

    row_major float4x4 prevViewProjMatrix;
    row_major float4x4 prevInvViewProjMatrix;
    row_major float4x4 prevUnjitteredViewProjMatrix;

    row_major float4x4 hiZCullViewProjMatrix;
    row_major float3x4 viewRotation; // World-to-view directions; fourth column is zero (16-byte row alignment).

    float3 cameraOrigin;
    float ndcFarZ;

    float3 cameraForward;
    uint flags;

    float2 ndcDepthScaleBias;
    float2 projectionZ;     // clip.z = view.z * projectionZ.x + projectionZ.y
    float4 projectionXY;    // clip.xy = view.xy * projectionXY.xy + clip.w * projectionXY.zw (including jitter).
    float4 uvToView;        // view.xy = (uv * uvToView.xy + uvToView.zw) * clip.w (top-left UV in [0, 1], independent of pass resolution).

    uint surfaceTableIndex;
    uint numLocalLights;
    uint lightCullingLightRefTableOffset;
    uint lightCullingZBinTableOffset;
    uint lightCullingTileWordTableOffset;
    float lightCullingZBinScale;
    float lightCullingZBinBias;
    uint lightCullingTileCountX;
    uint lightCullingTileCountY;
    uint lightCullingWordsPerTile;
    uint lightCullingDebugMode;
    uint padding0;

    row_major float3x4 envLightAxis;
    float3 envLightColor;
    float envLightIntensity;

    uint envLightDiffuseIrradianceCubeMapIndex;
    uint envLightSpecularCubeMapIndex;
    uint envLightSpecularMipCount;
    uint aoTextureIndex;

    ShaderCSMData csm;
    ShaderShadowAtlasData shadowAtlas;
};

#ifdef __cplusplus
static_assert(offsetof(CameraConstants, csm) == 704 && offsetof(CameraConstants, shadowAtlas) == 960, "Shadow atlas data must follow the existing camera constants");
static_assert(sizeof(CameraConstants) == 992, "CameraConstants must match the HLSL constant-buffer layout");
#endif

struct SurfaceDrawParams {
    uint instanceBaseOffset;
};

struct ShaderSkinningJoint {
    row_major float3x4 matrix;
};

struct ShaderSurface {
    row_major float3x4 worldMatrix;
    row_major float3x4 prevWorldMatrix;

    float4 localBoundingSphere;
    uint shaderMaterialIndex;
    uint flags;
    uint skinningJointOffset; // Element offset in the camera joint buffer; ShaderSkinningJointOffsetInvalid if unavailable.
    uint prevSkinningJointOffset; // Previous rendered pose; aliases the current offset when history is unavailable.
};

#ifdef __cplusplus
static_assert(sizeof(ShaderSkinningJoint) == 48, "ShaderSkinningJoint must match the joint buffer stride");
static_assert(sizeof(ShaderSurface) == 128, "ShaderSurface must match the HLSL structured-buffer layout");
#endif

struct ShaderMaterial {
    uint baseColorTextureIndex;
    uint normalTextureIndex;
    uint emissiveTextureIndex;
    uint metallicTextureIndex;
    uint roughnessTextureIndex;
    uint aoTextureIndex;
    uint clearCoatTextureIndex;
    uint clearCoatRoughnessTextureIndex;
    uint clearCoatNormalTextureIndex;
    uint heightTextureIndex;
    uint samplerIndex;
    uint streamingMaterialIndex;

    uint packedBaseColorFactor;
    float alphaCutoff;
    uint2 packedEmissionColorFactor;
    uint packedMetallicRoughnessFactors;
    float aoStrength;
    uint packedClearCoatFactors;
    float heightScale;
    uint flags; // ShaderMaterialFlag* bitmask.

#ifndef __cplusplus
    inline float4 GetBaseColorFactor() { return UnpackRGBA8UNormToFloat4(packedBaseColorFactor); }
    inline float4 GetEmissionColorFactor() { return UnpackRGBA16FToFloat4(packedEmissionColorFactor); }
    inline float2 GetMetallicRoughnessFactors() { return UnpackRG16UNormToFloat2(packedMetallicRoughnessFactors); }
    inline float2 GetClearCoatFactors() { return UnpackRG16UNormToFloat2(packedClearCoatFactors); }
#endif
};

#ifdef __cplusplus
static_assert(sizeof(ShaderMaterial) == 84, "ShaderMaterial must remain a tightly packed StructuredBuffer element");
#endif

struct ShaderLightRef {
    uint packedLightRef; // low 2 bits: light type, remaining bits: type-local light index
    uint packedTileMin;  // low 16 bits: tile X, high 16 bits: tile Y
    uint packedTileMax;  // low 16 bits: tile X, high 16 bits: tile Y
};

#ifdef __cplusplus
static_assert(sizeof(ShaderLightRef) == 12, "ShaderLightRef must remain a tightly packed StructuredBuffer element");
#endif

#endif // SHADERINTEROP_H
