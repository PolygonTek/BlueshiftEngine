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

#include "Precompiled.h"
#include "RenderCvars.h"
#include "RHI/RHI.h"
#include "Render/Frontend/RenderCamera.h"

CVAR(r_renderThread, "1", BE1::CVar::Flag::Bool, "enable the render thread (requires restart)");

CVAR(r_skipViewCulling, "0", BE1::CVar::Flag::Bool, "skip camera view culling for render lights, objects and mesh surfaces");
CVAR(r_skipDrawCamera2D, "0", BE1::CVar::Flag::Bool, "skip 2D draw camera render commands");
CVAR(r_skipDrawCamera3D, "0", BE1::CVar::Flag::Bool, "skip 3D draw camera render commands");
CVAR(r_skipPostProcess, "0", BE1::CVar::Flag::Bool, "skip post process dispatches");

CVAR(r_debugDrawMaxPrimitives, "16384", BE1::CVar::Flag::Integer, "maximum number of debug draw primitives");
CVAR(r_debugDrawMaxVertices, "262144", BE1::CVar::Flag::Integer, "maximum number of debug draw vertices");

CVAR(r_showLights2, "0", BE1::CVar::Flag::Bool, "show visible light bounding volumes");
CVAR(r_lightCullingZBin, "1", BE1::CVar::Flag::Bool, "enable light Z-bin culling in forward lighting");
CVAR_MINMAX(r_lightCullingDebug, "0", BE1::CVar::Flag::Integer, "light culling debug view (0 = off, 1 = tile mask, 2 = tile mask and Z-bin)", 0, 2);

CVAR(r_vsync, "0", BE1::CVar::Flag::Bool | BE1::CVar::Flag::Archive, "synchronize presentation to the vertical refresh rate");
CVAR(r_hdrOutput, "1", BE1::CVar::Flag::Bool | BE1::CVar::Flag::Archive, "enable HDR output when supported");
CVAR_MINMAX(r_hdrPaperWhiteNits, "200", BE1::CVar::Flag::Float | BE1::CVar::Flag::Archive, "HDR paper white luminance in nits", 80.0f, 500.0f);
CVAR_MINMAX(r_hdrPeakNits, "0", BE1::CVar::Flag::Float | BE1::CVar::Flag::Archive, "HDR peak luminance in nits (0 = use the display value)", 0.0f, 10000.0f);

CVAR(r_depthPrepass, "0", BE1::CVar::Flag::Bool, "enable depth prepass");
CVAR(r_snapshotColorHalfRes, "1", BE1::CVar::Flag::Bool | BE1::CVar::Flag::Archive, "capture non-MSAA scene snapshot color and depth at half resolution");
CVAR(r_specularEnergyCompensation2, "1", BE1::CVar::Flag::Bool | BE1::CVar::Flag::Archive, "use energy compensation for multiple scattering in a microfacet model");

// Specular AA
CVAR(r_specularAA, "0", BE1::CVar::Flag::Bool | BE1::CVar::Flag::Archive, "enable screen-space specular anti-aliasing");
CVAR_MINMAX(r_specularAAVariance, "0.25", BE1::CVar::Flag::Float | BE1::CVar::Flag::Archive, "screen-space normal variance scale for specular anti-aliasing", 0.0f, 1.0f);
CVAR_MINMAX(r_specularAALimit, "0.2", BE1::CVar::Flag::Float | BE1::CVar::Flag::Archive, "maximum additive squared perceptual roughness for specular anti-aliasing", 0.0f, 1.0f);

// Mesh LOD
CVAR(r_meshLOD, "1", BE1::CVar::Flag::Bool, "enable mesh LOD selection (0 = force LOD0, including explicit LOD requests)");
CVAR(r_meshLODPixelError2, "2.0", BE1::CVar::Flag::Float, "estimated mesh LOD pixel error tolerance, <= 0 uses LOD0");

// Shadows
CVAR(r_shadows2, "1", BE1::CVar::Flag::Bool | BE1::CVar::Flag::Archive, "enable shadows");
CVAR_MINMAX(r_shadowDebug, "0", BE1::CVar::Flag::Integer, "shadow debug view (0 = off, 1 = visibility, 2 = coverage)", 0, 2);
CVAR_MINMAX(r_shadowMapQuality2, "2", BE1::CVar::Flag::Integer | BE1::CVar::Flag::Archive, "shadow map filter quality (0 = 1, 1 = 5, 2 = 9, 3 = 16 rotated Poisson comparisons)", 0, 3);
CVAR(r_shadowMapRotationUseTexture, "1", BE1::CVar::Flag::Bool | BE1::CVar::Flag::Archive, "use STBN rotation instead of a fixed-seed hash for Q3 Poisson shadows; animate with TAA (rebuilds shader pipelines)");

// Local Shadows
CVAR_MINMAX(r_shadowAtlasSize, "8192", BE1::CVar::Flag::Integer, "local shadow atlas side length in texels; rounded down to a power of two when captured", 1024, 8192);
CVAR_MINMAX(r_shadowAtlasMaxTileResolution, "2048", BE1::CVar::Flag::Integer, "maximum local shadow tile resolution including border (per face for point lights); rounded down to a power of two when captured", RenderFrameSettings::ShadowAtlasMinTileResolution, 2048);
CVAR_MINMAX(r_shadowAtlasMaxViews, "180", BE1::CVar::Flag::Integer, "maximum local shadow views per camera (spot = 1, point = 6, 0 = disabled)", 0, 1024);
CVAR(r_shadowAtlasLoadBalancing, "1", BE1::CVar::Flag::Bool | BE1::CVar::Flag::Archive, "double the local shadow update interval for each halving of projected size below r_shadowMapUpdateMinProjectedSize");
CVAR_MINMAX(r_shadowMapProjectedSizeScaleSpot, "0.65", BE1::CVar::Flag::Float, "projected light size multiplier for spot shadow resolution selection (does not affect the minimum projected size cutoff)", 0.125f, 4.0f);
CVAR_MINMAX(r_shadowMapProjectedSizeScalePoint, "0.5", BE1::CVar::Flag::Float, "projected light size multiplier for point shadow resolution selection (does not affect the minimum projected size cutoff)", 0.125f, 4.0f);
CVAR_MINMAX(r_shadowMapUpdateMinProjectedSize, "512", BE1::CVar::Flag::Integer, "minimum projected light size for every-frame Point/Spot shadow updates (unscaled long edge in internal render pixels)", RenderFrameSettings::LocalShadowMinProjectedSize, 16384);
CVAR_MINMAX(r_shadowMapFilterRadius, "2.25", BE1::CVar::Flag::Float, "Point/Spot Q3 Poisson maximum sample radius in texels at 1024 resolution; adjusted for tile resolution (Q0/Q1/Q2 use fixed kernels)", RenderFrameSettings::ShadowMapReferenceMinRadius, RenderFrameSettings::ShadowMapReferenceMaxRadius);

// CSM
CVAR_MINMAX(r_csmResolution, "1024", BE1::CVar::Flag::Integer | BE1::CVar::Flag::Archive, "cascade shadow map resolution", 128, 8192);
CVAR_MINMAX(r_csmFilterRadius, "1.75", BE1::CVar::Flag::Float, "CSM Q3 Poisson maximum sample radius in texels at 1024 resolution; adjusted for resolution and cascade scale (Q0/Q1/Q2 use fixed kernels)", RenderFrameSettings::ShadowMapReferenceMinRadius, RenderFrameSettings::ShadowMapReferenceMaxRadius);
CVAR_MINMAX(r_csmCount, "4", BE1::CVar::Flag::Integer | BE1::CVar::Flag::Archive, "active CSM cascade count", 1, RenderFrameSettings::CSMCascadeMaxCount);
CVAR_MINMAX(r_csmSplitLambda, "0.9", BE1::CVar::Flag::Float | BE1::CVar::Flag::Archive, "CSM split distribution (0 = uniform, 1 = logarithmic; orthographic cameras use uniform)", 0.0f, 1.0f);
CVAR_MINMAX(r_csmMaxDistance, "150", BE1::CVar::Flag::Float | BE1::CVar::Flag::Archive, "maximum CSM distance in world units (0 = disabled)", 0.0f, 10000.0f);
CVAR_MINMAX(r_csmBlendFraction, "0.05", BE1::CVar::Flag::Float | BE1::CVar::Flag::Archive, "fraction of each CSM map's normalized safe coverage used for cascade blending", 0.0f, 0.16f);
CVAR_MINMAX(r_csmFadeFraction, "0.1", BE1::CVar::Flag::Float | BE1::CVar::Flag::Archive, "fraction of CSM receiver range used for distance fading", 0.01f, 1.0f);
CVAR(r_csmConstantDepthBias0, "60000.0", BE1::CVar::Flag::Float, "constant depth bias for CSM cascade 0");
CVAR(r_csmConstantDepthBias1, "22500.0", BE1::CVar::Flag::Float, "constant depth bias for CSM cascade 1");
CVAR(r_csmConstantDepthBias2, "7800.0", BE1::CVar::Flag::Float, "constant depth bias for CSM cascade 2");
CVAR(r_csmConstantDepthBias3, "2100.0", BE1::CVar::Flag::Float, "constant depth bias for CSM cascade 3");
CVAR(r_csmSlopeScaledDepthBias0, "1.0", BE1::CVar::Flag::Float, "slope-scaled depth bias for CSM cascade 0");
CVAR(r_csmSlopeScaledDepthBias1, "1.0", BE1::CVar::Flag::Float, "slope-scaled depth bias for CSM cascade 1");
CVAR(r_csmSlopeScaledDepthBias2, "1.0", BE1::CVar::Flag::Float, "slope-scaled depth bias for CSM cascade 2");
CVAR(r_csmSlopeScaledDepthBias3, "1.0", BE1::CVar::Flag::Float, "slope-scaled depth bias for CSM cascade 3");

// MSAA
CVAR_MINMAX(r_msaa, "1", BE1::CVar::Flag::Integer | BE1::CVar::Flag::Archive, "MSAA sample count (1 = off, 2/4/8 = enabled)", 0, 8);

// TAA
CVAR(r_taa, "1", BE1::CVar::Flag::Bool | BE1::CVar::Flag::Archive, "enable temporal anti-aliasing");
CVAR(r_taaJitter, "1", BE1::CVar::Flag::Bool, "enable temporal projection jitter");
CVAR_MINMAX(r_taaJitterSampleCount, "16", BE1::CVar::Flag::Integer, "temporal projection jitter sample count", 1, 64);
CVAR_MINMAX(r_taaHistoryWeight, "0.95", BE1::CVar::Flag::Float, "temporal anti-aliasing history blend weight", 0.0f, 1.0f);
CVAR_MINMAX(r_taaColorBoxSigma, "2.0", BE1::CVar::Flag::Float, "temporal anti-aliasing variance color box sigma scale", 0.0f, 15.0f);
CVAR_MINMAX(r_taaSubpixelWeight, "0.5", BE1::CVar::Flag::Float, "temporal anti-aliasing maximum subpixel current frame weight", 0.0f, 1.0f);
CVAR_MINMAX(r_taaLuminanceWeight, "0.25", BE1::CVar::Flag::Float, "temporal anti-aliasing luminance mismatch history rejection weight", 0.0f, 1.0f);
CVAR_MINMAX(r_taaDepthFallbackThreshold, "1.0", BE1::CVar::Flag::Float, "temporal anti-aliasing depth fallback linear depth threshold", 0.0f, 10000.0f);

// Hi-Z Occlusion Culling
CVAR(r_hiZOcclusionCulling, "0", BE1::CVar::Flag::Bool, "enable Hi-Z occlusion culling");
CVAR(r_hiZOcclusionCullingFreeze, "0", BE1::CVar::Flag::Bool, "freeze Hi-Z occlusion culling depth pyramid updates");
CVAR(r_hiZDebugPreview, "0", BE1::CVar::Flag::Bool, "enable Hi-Z debug preview texture update");
CVAR_MINMAX(r_hiZDebugPreviewMipLevel, "0", BE1::CVar::Flag::Integer, "Hi-Z debug preview mip level", 0, RenderHiZMaxLevelCount - 1);

// Ambient Occlusion
CVAR_MINMAX(r_ao, "1", BE1::CVar::Flag::Integer | BE1::CVar::Flag::Archive, "ambient occlusion mode: 0 = off, 1 = GTAO", 0, 1);
CVAR_MINMAX(r_aoRadius, "0.5", BE1::CVar::Flag::Float, "ambient occlusion radius in world units", 0.01f, 10.0f);
CVAR(r_aoHalfResolution, "1", BE1::CVar::Flag::Bool | BE1::CVar::Flag::Archive, "compute ambient occlusion at half resolution");
CVAR(r_aoTemporalNoise, "1", BE1::CVar::Flag::Bool | BE1::CVar::Flag::Archive, "vary the ambient occlusion sampling pattern each frame independently of TAA; AO debug keeps a fixed pattern");
CVAR(r_aoMultiBounce, "0", BE1::CVar::Flag::Bool | BE1::CVar::Flag::Archive, "approximate diffuse multi-bounce lighting for material and screen-space AO (recompiles shaders)");

// GTAO
CVAR_MINMAX(r_gtaoQuality, "2", BE1::CVar::Flag::Integer | BE1::CVar::Flag::Archive, "GTAO quality: 0 = low, 1 = medium, 2 = high, 3 = ultra", 0, 3);
CVAR_MINMAX(r_gtaoDenoisePasses, "2", BE1::CVar::Flag::Integer | BE1::CVar::Flag::Archive, "GTAO edge-aware denoise pass count", 0, 4);
CVAR(r_gtao32BitDepth, "0", BE1::CVar::Flag::Bool | BE1::CVar::Flag::Archive, "use 32-bit GTAO prefiltered depth instead of 16-bit");
CVAR_MINMAX(r_gtaoDebug, "0", BE1::CVar::Flag::Integer, "GTAO debug source: 0 = off, 1 = final, 2 = raw, 3 = edges, 4..8 = depth mip 0..4", 0, 8);
CVAR_MINMAX(r_gtaoPower, "1.0", BE1::CVar::Flag::Float | BE1::CVar::Flag::Archive, "GTAO final visibility power", 0.25f, 4.0f);
CVAR_MINMAX(r_gtaoFalloffRange, "0.615", BE1::CVar::Flag::Float, "GTAO effect falloff range", 0.0f, 1.0f);
CVAR_MINMAX(r_gtaoRadiusMultiplier, "1.457", BE1::CVar::Flag::Float, "GTAO effect radius multiplier", 0.3f, 3.0f);
CVAR_MINMAX(r_gtaoSampleDistributionPower, "2.0", BE1::CVar::Flag::Float, "GTAO sample distribution power", 1.0f, 3.0f);
CVAR_MINMAX(r_gtaoThinOccluderCompensation, "0.0", BE1::CVar::Flag::Float, "GTAO thin occluder compensation", 0.0f, 1.0f);
CVAR_MINMAX(r_gtaoDepthMipSamplingOffset, "3.3", BE1::CVar::Flag::Float, "GTAO depth mip sampling offset", 0.0f, 5.0f);

// SSR
CVAR(r_ssr, "1", BE1::CVar::Flag::Bool | BE1::CVar::Flag::Archive, "enable screen-space reflections");
CVAR_MINMAX(r_ssrResolutionMode, "1", BE1::CVar::Flag::Integer | BE1::CVar::Flag::Archive, "SSR resolution mode: 0 = full, 1 = half, 2 = quarter (per axis)", 0, 2);
CVAR(r_ssrTransparent, "1", BE1::CVar::Flag::Bool | BE1::CVar::Flag::Archive, "enable screen-space reflections on transparent surfaces");
CVAR_MINMAX(r_ssrDebug, "0", BE1::CVar::Flag::Integer, "SSR debug source: 0 = off, 1 = raw reflection, 2 = upsampled reflection, 3 = trace status, 4 = history UV, 5 = confidence, 6 = iterations", 0, 6);
CVAR(r_ssrDebugTransparent, "0", BE1::CVar::Flag::Bool, "debug transparent SSR instead of opaque SSR");
CVAR_MINMAX(r_ssrTraceMaxIterations, "64", BE1::CVar::Flag::Integer, "screen-space reflection maximum Hi-Z traversal iterations", 1, 128);
CVAR_MINMAX(r_ssrTraceThickness, "0.5", BE1::CVar::Flag::Float, "screen-space reflection hit thickness", 0.0f, 10.0f);
CVAR_MINMAX(r_ssrRayRoughnessScale, "0.5", BE1::CVar::Flag::Float, "SSR GGX linear roughness scale for stochastic ray directions, balanced with cone filtering", 0.0f, 1.0f);
CVAR_MINMAX(r_ssrScreenEdgeFade, "0.025", BE1::CVar::Flag::Float, "SSR screen-edge fade width relative to viewport height", 0.0f, 0.5f);
CVAR_MINMAX(r_ssrRoughnessFadeStart, "0.6", BE1::CVar::Flag::Float, "SSR roughness where reflection confidence starts fading", 0.0f, 1.0f);
CVAR_MINMAX(r_ssrRoughnessFadeEnd, "0.8", BE1::CVar::Flag::Float, "SSR roughness where reflection confidence reaches zero and tracing stops", 0.0f, 1.0f);
CVAR(r_ssrTemporal, "1", BE1::CVar::Flag::Bool, "enable SSR temporal accumulation");
CVAR_MINMAX(r_ssrTemporalHistoryWeight, "0.95", BE1::CVar::Flag::Float, "SSR temporal history weight", 0.0f, 0.99f);

// Texture
CVAR_MINMAX(r_textureFilter, "4", BE1::CVar::Flag::Integer | BE1::CVar::Flag::Archive, "texture filtering mode for material samplers", 0, static_cast<int>(RHI::TextureFilter::Count) - 1);
CVAR_MINMAX(r_textureMaxAnisotropy, "8", BE1::CVar::Flag::Integer | BE1::CVar::Flag::Archive, "maximum anisotropy for material samplers", 1, 16);
CVAR(r_textureStreamingStreamInDelay, "0", BE1::CVar::Flag::Integer, "texture streaming frame delay before loading higher-resolution mips");
CVAR(r_textureStreamingMaxRequestsPerFrame, "256", BE1::CVar::Flag::Integer, "maximum texture streaming requests per frame");
CVAR_MINMAX(r_textureStreamingMaxLevelReduction, "6", BE1::CVar::Flag::Integer, "maximum mip level reduction for streaming textures", 0, 16);
CVAR(r_textureStreamingMinResidentMaxDimension, "64", BE1::CVar::Flag::Integer, "minimum requested resident texture max dimension");

// Tone Mapping
CVAR(r_toneMapping, "1", BE1::CVar::Flag::Bool, "enable compute tone mapping");
CVAR_MINMAX(r_toneMapOp, "7", BE1::CVar::Flag::Integer, "tone mapping operator", 0, 11);
CVAR(r_toneMapLuminanceLogMin, "-10", BE1::CVar::Flag::Float, "tone mapping luminance histogram minimum log2 luminance");
CVAR(r_toneMapLuminanceLogMax, "2", BE1::CVar::Flag::Float, "tone mapping luminance histogram maximum log2 luminance");
CVAR_MINMAX(r_toneMapLuminanceAdaptationRate, "1.1", BE1::CVar::Flag::Float, "tone mapping luminance adaptation rate", 0.0f, 100.0f);
CVAR_MINMAX(r_toneMapMiddleGray, "0.18", BE1::CVar::Flag::Float, "tone mapping middle gray", 0.0f, 10.0f);
CVAR_MINMAX(r_toneMapExposureCompensation, "0", BE1::CVar::Flag::Float, "tone mapping exposure compensation in EV", -5.0f, 5.0f);
CVAR_MINMAX(r_toneMapMinExposureEV, "-8", BE1::CVar::Flag::Float, "tone mapping minimum automatic exposure in EV", -16.0f, 16.0f);
CVAR_MINMAX(r_toneMapMaxExposureEV, "2", BE1::CVar::Flag::Float, "tone mapping maximum automatic exposure in EV", -16.0f, 16.0f);
CVAR_MINMAX(r_whiteBalanceTemperature, "6500", BE1::CVar::Flag::Float, "white balance color temperature in Kelvin", 1000.0f, 15000.0f);

// Color Grading
CVAR(r_colorGrading, "0", BE1::CVar::Flag::Bool, "enable 3D LUT color grading");
CVAR_MINMAX(r_colorGradingSaturation, "1", BE1::CVar::Flag::Float, "color grading saturation", 0.0f, 2.0f);
CVAR_MINMAX(r_colorGradingContrast, "1", BE1::CVar::Flag::Float, "color grading contrast", 0.0f, 2.0f);
CVAR_MINMAX(r_colorGradingGamma, "1", BE1::CVar::Flag::Float, "color grading gamma", 0.1f, 4.0f);
CVAR_MINMAX(r_colorGradingGain, "1", BE1::CVar::Flag::Float, "color grading gain", 0.0f, 4.0f);
CVAR_MINMAX(r_colorGradingTintR, "1", BE1::CVar::Flag::Float, "color grading tint red multiplier", 0.0f, 2.0f);
CVAR_MINMAX(r_colorGradingTintG, "1", BE1::CVar::Flag::Float, "color grading tint green multiplier", 0.0f, 2.0f);
CVAR_MINMAX(r_colorGradingTintB, "1", BE1::CVar::Flag::Float, "color grading tint blue multiplier", 0.0f, 2.0f);

// Image Effects
CVAR_MINMAX(r_lensDistortion, "0", BE1::CVar::Flag::Float, "lens distortion strength (positive = barrel, negative = pincushion)", -0.5f, 0.5f);
CVAR_MINMAX(r_chromaticAberration, "0", BE1::CVar::Flag::Float, "chromatic aberration channel offset at the screen edge in pixels", 0.0f, 32.0f);
CVAR_MINMAX(r_vignette, "0", BE1::CVar::Flag::Float, "vignette intensity", 0.0f, 1.0f);
CVAR(r_dithering, "1", BE1::CVar::Flag::Bool, "enable final output dithering");

// Bloom
CVAR_MINMAX(r_bloom, "2", BE1::CVar::Flag::Integer | BE1::CVar::Flag::Archive, "bloom mode (0 = off, 1 = pyramid, 2 = convolution)", RenderBloomMode::Off, RenderBloomMode::Convolution);
CVAR_MINMAX(r_bloomThreshold, "1.0", BE1::CVar::Flag::Float, "bloom luminance threshold", 0.0f, 100.0f);
CVAR_MINMAX(r_bloomSoftThreshold, "0.5", BE1::CVar::Flag::Float, "bloom soft threshold width", 0.0f, 10.0f);
CVAR_MINMAX(r_bloomIntensity, "0.15", BE1::CVar::Flag::Float, "final bloom composite intensity", 0.0f, 10.0f);
CVAR_MINMAX(r_bloomDirtIntensity, "0", BE1::CVar::Flag::Float | BE1::CVar::Flag::Archive, "bloom dirt mask intensity", 0.0f, 10.0f);

// Pyramid Bloom
CVAR_MINMAX(r_pyramidBloomPrefilterQuality, "1", BE1::CVar::Flag::Integer, "pyramid bloom prefilter quality (0 = low quality, 1 = high quality)", 0, 1);
CVAR_MINMAX(r_pyramidBloomScatter, "0.67", BE1::CVar::Flag::Float, "pyramid bloom mip scatter weight", 0.0f, 1.0f);
CVAR_MINMAX(r_pyramidBloomPrefilterRadius, "1.25", BE1::CVar::Flag::Float, "pyramid bloom prefilter sample radius", 0.0f, 10.0f);
CVAR_MINMAX(r_pyramidBloomDownsampleRadius, "1.0", BE1::CVar::Flag::Float, "pyramid bloom downsample sample radius", 0.0f, 10.0f);
CVAR_MINMAX(r_pyramidBloomUpsampleRadius, "1.0", BE1::CVar::Flag::Float, "pyramid bloom upsample sample radius", 0.0f, 10.0f);
CVAR_MINMAX(r_pyramidBloomMipCount, "6", BE1::CVar::Flag::Integer, "pyramid bloom mip level count", 1, RenderBloomDownMaxLevelCount);

// Convolution Bloom
CVAR_MINMAX(r_convolutionBloomFFTMaxSize, "1024", BE1::CVar::Flag::Integer | BE1::CVar::Flag::Archive, "maximum convolution bloom FFT texture axis size", 256, 1024);
CVAR_MINMAX(r_convolutionBloomPrefilterRadius, "1.5", BE1::CVar::Flag::Float, "convolution bloom prefilter sample radius", 0.0f, 10.0f);
CVAR_MINMAX(r_convolutionBloomKernelScale, "1.0", BE1::CVar::Flag::Float, "convolution bloom kernel scale", 0.01f, 10.0f);
CVAR_MINMAX(r_convolutionBloomKernelGain, "1.0", BE1::CVar::Flag::Float, "convolution bloom normalized kernel gain", 0.0f, 10.0f);
CVAR(r_convolutionBloomKernelRotation, "0", BE1::CVar::Flag::Float, "convolution bloom kernel rotation in degrees");
CVAR_MINMAX(r_convolutionBloomPaddingTrim, "0.125", BE1::CVar::Flag::Float, "convolution bloom padding reduction as fraction of FFT size", 0.0f, 0.5f);
CVAR_MINMAX(r_convolutionBloomApertureScale, "0.2", BE1::CVar::Flag::Float, "convolution bloom aperture size within the FFT domain", 0.02f, 1.0f);
CVAR_MINMAX(r_convolutionBloomDispersionAmount, "1.0", BE1::CVar::Flag::Float, "convolution bloom diffraction dispersion amount", 0.0f, 1.0f);
CVAR(r_convolutionBloomDebug, "1", BE1::CVar::Flag::Bool, "save convolution bloom kernel bounds debug image");

static uint32_t NormalizeMainRTSampleCount(int sampleCount) {
    if (sampleCount >= 8) {
        return 8;
    }
    if (sampleCount >= 4) {
        return 4;
    }
    if (sampleCount >= 2) {
        return 2;
    }
    return 1;
}

RenderFrameSettings CaptureRenderFrameSettings() {
    assert(BE1::Engine::IsInMainThread());

    RenderFrameSettings settings;

    settings.mainRTSampleCount = NormalizeMainRTSampleCount(r_msaa.GetInteger());

    settings.shadows = r_shadows2.GetBool();
    settings.shadowDebugMode = static_cast<uint32_t>(r_shadowDebug.GetInteger());
    settings.shadowMapQuality = static_cast<uint32_t>(r_shadowMapQuality2.GetInteger());

    // Atlas 크기와 타일 해상도는 예산을 초과하지 않도록 2 의 거듭제곱으로 내림한다.
    settings.shadowAtlasSize = BE1::Math::RoundDownPowerOfTwo(static_cast<uint32_t>(BE1::Clamp(r_shadowAtlasSize.GetInteger(), 1024, 8192)));
    const int maxTileResolution = BE1::Min(static_cast<int>(settings.shadowAtlasSize), static_cast<int>(r_shadowAtlasMaxTileResolution.GetMaxValue()));
    settings.shadowAtlasMaxTileResolution = BE1::Math::RoundDownPowerOfTwo(static_cast<uint32_t>(BE1::Clamp(r_shadowAtlasMaxTileResolution.GetInteger(), static_cast<int>(RenderFrameSettings::ShadowAtlasMinTileResolution), maxTileResolution)));
    settings.shadowAtlasMaxViews = static_cast<uint32_t>(BE1::Clamp(r_shadowAtlasMaxViews.GetInteger(), 0, 1024));
    settings.shadowAtlasLoadBalancing = r_shadowAtlasLoadBalancing.GetBool();
    settings.shadowMapProjectedSizeScaleSpot = r_shadowMapProjectedSizeScaleSpot.GetFloat();
    settings.shadowMapProjectedSizeScalePoint = r_shadowMapProjectedSizeScalePoint.GetFloat();
    settings.shadowMapUpdateMinProjectedSize = static_cast<uint32_t>(BE1::Clamp(r_shadowMapUpdateMinProjectedSize.GetInteger(), static_cast<int>(RenderFrameSettings::LocalShadowMinProjectedSize), 16384));
    settings.shadowMapFilterRadius = r_shadowMapFilterRadius.GetFloat();

    settings.csmResolution = static_cast<uint32_t>(r_csmResolution.GetInteger());
    settings.csmFilterRadius = r_csmFilterRadius.GetFloat();
    settings.csmCascadeCount = static_cast<uint32_t>(r_csmCount.GetInteger());
    settings.csmSplitLambda = r_csmSplitLambda.GetFloat();
    settings.csmMaxDistance = r_csmMaxDistance.GetFloat();
    settings.csmBlendFraction = r_csmBlendFraction.GetFloat();
    settings.csmFadeFraction = r_csmFadeFraction.GetFloat();
    settings.csmConstantDepthBias[0] = r_csmConstantDepthBias0.GetFloat();
    settings.csmConstantDepthBias[1] = r_csmConstantDepthBias1.GetFloat();
    settings.csmConstantDepthBias[2] = r_csmConstantDepthBias2.GetFloat();
    settings.csmConstantDepthBias[3] = r_csmConstantDepthBias3.GetFloat();
    settings.csmSlopeScaledDepthBias[0] = r_csmSlopeScaledDepthBias0.GetFloat();
    settings.csmSlopeScaledDepthBias[1] = r_csmSlopeScaledDepthBias1.GetFloat();
    settings.csmSlopeScaledDepthBias[2] = r_csmSlopeScaledDepthBias2.GetFloat();
    settings.csmSlopeScaledDepthBias[3] = r_csmSlopeScaledDepthBias3.GetFloat();

    settings.taaJitterSampleCount = static_cast<uint32_t>(r_taaJitterSampleCount.GetInteger());
    settings.hiZDebugPreviewMipLevel = static_cast<uint32_t>(r_hiZDebugPreviewMipLevel.GetInteger());
    settings.lightCullingDebugMode = static_cast<uint32_t>(r_lightCullingDebug.GetInteger());
    settings.toneMappingOperator = static_cast<uint32_t>(r_toneMapOp.GetInteger());
    settings.pyramidBloomMipMaxLevelCount = static_cast<uint32_t>(r_pyramidBloomMipCount.GetInteger());
    settings.convolutionBloomFFTMaxSize = static_cast<uint32_t>(r_convolutionBloomFFTMaxSize.GetInteger());

    settings.bloomMode = static_cast<RenderBloomMode>(r_bloom.GetInteger());

    settings.hdrPaperWhiteNits = r_hdrPaperWhiteNits.GetFloat();
    settings.hdrPeakNits = r_hdrPeakNits.GetFloat();

    settings.meshLOD = r_meshLOD.GetBool();
    settings.meshLODPixelError = r_meshLODPixelError2.GetFloat();

    settings.toneMapLuminanceLogMin = r_toneMapLuminanceLogMin.GetFloat();
    settings.toneMapLuminanceLogMax = BE1::Max(r_toneMapLuminanceLogMax.GetFloat(), settings.toneMapLuminanceLogMin + 0.0001f);
    settings.toneMapLuminanceAdaptationRate = r_toneMapLuminanceAdaptationRate.GetFloat();
    settings.toneMapMiddleGray = r_toneMapMiddleGray.GetFloat();
    settings.toneMapExposureCompensation = r_toneMapExposureCompensation.GetFloat();
    settings.toneMapMinExposureEV = r_toneMapMinExposureEV.GetFloat();
    settings.toneMapMaxExposureEV = BE1::Max(r_toneMapMaxExposureEV.GetFloat(), settings.toneMapMinExposureEV);
    settings.whiteBalanceTemperature = r_whiteBalanceTemperature.GetFloat();

    settings.taaHistoryWeight = r_taaHistoryWeight.GetFloat();
    settings.taaColorBoxSigma = r_taaColorBoxSigma.GetFloat();
    settings.taaSubpixelWeight = r_taaSubpixelWeight.GetFloat();
    settings.taaLuminanceWeight = r_taaLuminanceWeight.GetFloat();
    settings.taaDepthFallbackThreshold = r_taaDepthFallbackThreshold.GetFloat();

    settings.lensDistortion = r_lensDistortion.GetFloat();
    settings.chromaticAberration = r_chromaticAberration.GetFloat();
    settings.vignetteIntensity = r_vignette.GetFloat();

    settings.bloomThreshold = r_bloomThreshold.GetFloat();
    settings.bloomSoftThreshold = r_bloomSoftThreshold.GetFloat();
    settings.bloomIntensity = r_bloomIntensity.GetFloat();
    settings.bloomDirtIntensity = r_bloomDirtIntensity.GetFloat();

    settings.pyramidBloomPrefilterQuality = static_cast<float>(r_pyramidBloomPrefilterQuality.GetInteger());
    settings.pyramidBloomScatter = r_pyramidBloomScatter.GetFloat();
    settings.pyramidBloomPrefilterRadius = r_pyramidBloomPrefilterRadius.GetFloat();
    settings.pyramidBloomDownsampleRadius = r_pyramidBloomDownsampleRadius.GetFloat();
    settings.pyramidBloomUpsampleRadius = r_pyramidBloomUpsampleRadius.GetFloat();

    settings.convolutionBloomPrefilterRadius = r_convolutionBloomPrefilterRadius.GetFloat();
    settings.convolutionBloomKernelScale = r_convolutionBloomKernelScale.GetFloat();
    settings.convolutionBloomKernelGain = r_convolutionBloomKernelGain.GetFloat();
    settings.convolutionBloomKernelRotation = r_convolutionBloomKernelRotation.GetFloat();
    settings.convolutionBloomPaddingTrim = r_convolutionBloomPaddingTrim.GetFloat();

    settings.skipViewCulling = r_skipViewCulling.GetBool();
    settings.skipDrawCamera2D = r_skipDrawCamera2D.GetBool();
    settings.skipDrawCamera3D = r_skipDrawCamera3D.GetBool();
    settings.skipPostProcess = r_skipPostProcess.GetBool();

    settings.showDebugLights = r_showLights2.GetBool();
    settings.lightCullingZBin = r_lightCullingZBin.GetBool();
    settings.depthPrepass = r_depthPrepass.GetBool();
    settings.specularAA = r_specularAA.GetBool();
    settings.specularAAVariance = r_specularAAVariance.GetFloat();
    settings.specularAALimit = r_specularAALimit.GetFloat();

    settings.vsync = r_vsync.GetBool();
    settings.hdrOutput = r_hdrOutput.GetBool();

    settings.snapshotColorHalfRes = r_snapshotColorHalfRes.GetBool();
    settings.taa = r_taa.GetBool();
    settings.taaJitter = r_taaJitter.GetBool();

    settings.hiZOcclusionCulling = r_hiZOcclusionCulling.GetBool();
    settings.hiZOcclusionCullingFreeze = r_hiZOcclusionCullingFreeze.GetBool();
    settings.hiZDebugPreview = r_hiZDebugPreview.GetBool();

    settings.aoMode = static_cast<RenderAOMode>(r_ao.GetInteger());
    settings.aoRadius = r_aoRadius.GetFloat();
    settings.aoHalfResolution = r_aoHalfResolution.GetBool();
    settings.aoTemporalNoise = r_aoTemporalNoise.GetBool();

    settings.gtaoQuality = static_cast<uint32_t>(r_gtaoQuality.GetInteger());
    settings.gtaoDenoisePasses = static_cast<uint32_t>(r_gtaoDenoisePasses.GetInteger());
    settings.gtao32BitDepth = r_gtao32BitDepth.GetBool();
    settings.gtaoDebug = static_cast<uint32_t>(r_gtaoDebug.GetInteger());
    settings.gtaoPower = r_gtaoPower.GetFloat();
    settings.gtaoFalloffRange = r_gtaoFalloffRange.GetFloat();
    settings.gtaoRadiusMultiplier = r_gtaoRadiusMultiplier.GetFloat();
    settings.gtaoSampleDistributionPower = r_gtaoSampleDistributionPower.GetFloat();
    settings.gtaoThinOccluderCompensation = r_gtaoThinOccluderCompensation.GetFloat();
    settings.gtaoDepthMipSamplingOffset = r_gtaoDepthMipSamplingOffset.GetFloat();

    settings.ssr = r_ssr.GetBool();
    settings.ssrResolutionMode = static_cast<uint32_t>(r_ssrResolutionMode.GetInteger());
    settings.ssrTransparent = r_ssrTransparent.GetBool();
    settings.ssrTemporal = r_ssrTemporal.GetBool();
    settings.ssrDebugMode = static_cast<uint32_t>(r_ssrDebug.GetInteger());
    settings.ssrDebugTransparent = r_ssrDebugTransparent.GetBool();
    settings.ssrTraceMaxIterations = r_ssrTraceMaxIterations.GetInteger();
    settings.ssrTraceThickness = r_ssrTraceThickness.GetFloat();
    settings.ssrRayRoughnessScale = r_ssrRayRoughnessScale.GetFloat();
    settings.ssrScreenEdgeFade = r_ssrScreenEdgeFade.GetFloat();
    settings.ssrRoughnessFadeEnd = r_ssrRoughnessFadeEnd.GetFloat();
    settings.ssrRoughnessFadeStart = BE1::Min(r_ssrRoughnessFadeStart.GetFloat(), settings.ssrRoughnessFadeEnd);
    settings.ssrTemporalHistoryWeight = r_ssrTemporalHistoryWeight.GetFloat();

    settings.toneMapping = r_toneMapping.GetBool();
    settings.colorGrading = r_colorGrading.GetBool();
    settings.dithering = r_dithering.GetBool();

    return settings;
}
