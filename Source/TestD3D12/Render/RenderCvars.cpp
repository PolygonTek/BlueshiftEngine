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
CVAR(r_frameComposition, "1", BE1::CVar::Flag::Bool, "route swap chain output through the frame composition texture");
CVAR(r_depthPrepass, "0", BE1::CVar::Flag::Bool, "enable depth prepass");
CVAR(r_specularEnergyCompensation2, "1", BE1::CVar::Flag::Bool | BE1::CVar::Flag::Archive, "use energy compensation for multiple scattering in a microfacet model");

CVAR_MINMAX(r_msaa, "1", BE1::CVar::Flag::Integer | BE1::CVar::Flag::Archive, "MSAA sample count (1 = off, 2/4/8 = enabled)", 0, 8);
CVAR(r_taa, "0", BE1::CVar::Flag::Bool | BE1::CVar::Flag::Archive, "enable temporal anti-aliasing");
CVAR(r_taaJitter, "1", BE1::CVar::Flag::Bool, "enable temporal projection jitter");
CVAR_MINMAX(r_taaJitterSampleCount, "16", BE1::CVar::Flag::Integer, "temporal projection jitter sample count", 1, 64);
CVAR_MINMAX(r_taaHistoryWeight, "0.9", BE1::CVar::Flag::Float, "temporal anti-aliasing history blend weight", 0.0f, 1.0f);
CVAR_MINMAX(r_taaColorBoxSigma, "1.0", BE1::CVar::Flag::Float, "temporal anti-aliasing variance color box sigma scale", 0.0f, 15.0f);
CVAR_MINMAX(r_taaSubpixelWeight, "0.8", BE1::CVar::Flag::Float, "temporal anti-aliasing maximum subpixel current frame weight", 0.0f, 1.0f);
CVAR_MINMAX(r_taaLuminanceWeight, "0.25", BE1::CVar::Flag::Float, "temporal anti-aliasing luminance mismatch history rejection weight", 0.0f, 1.0f);
CVAR_MINMAX(r_taaDepthFallbackThreshold, "1.0", BE1::CVar::Flag::Float, "temporal anti-aliasing depth fallback linear depth threshold", 0.0f, 10000.0f);

CVAR(r_hiZOcclusionCulling, "0", BE1::CVar::Flag::Bool, "enable Hi-Z occlusion culling");
CVAR(r_hiZOcclusionCullingFreeze, "0", BE1::CVar::Flag::Bool, "freeze Hi-Z occlusion culling depth pyramid updates");
CVAR(r_hiZDebugPreview, "0", BE1::CVar::Flag::Bool, "enable Hi-Z debug preview texture update");
CVAR_MINMAX(r_hiZDebugPreviewMipLevel, "0", BE1::CVar::Flag::Integer, "Hi-Z debug preview mip level", 0, RenderHiZMaxLevelCount - 1);

CVAR_MINMAX(r_textureFilter, "3", BE1::CVar::Flag::Integer | BE1::CVar::Flag::Archive, "texture filtering mode for material samplers", 0, static_cast<int>(RHI::TextureFilter::Count) - 1);
CVAR_MINMAX(r_textureMaxAnisotropy, "8", BE1::CVar::Flag::Integer | BE1::CVar::Flag::Archive, "maximum anisotropy for material samplers", 1, 16);
CVAR(r_textureStreamingStreamInDelay, "0", BE1::CVar::Flag::Integer, "texture streaming frame delay before loading higher-resolution mips");
CVAR(r_textureStreamingMaxRequestsPerFrame, "256", BE1::CVar::Flag::Integer, "maximum texture streaming requests per frame");
CVAR_MINMAX(r_textureStreamingMaxLevelReduction, "6", BE1::CVar::Flag::Integer, "maximum mip level reduction for streaming textures", 0, 16);
CVAR(r_textureStreamingMinResidentMaxDimension, "64", BE1::CVar::Flag::Integer, "minimum requested resident texture max dimension");

CVAR(r_toneMapping, "1", BE1::CVar::Flag::Bool, "enable compute tone mapping");
CVAR_MINMAX(r_toneMapOp, "7", BE1::CVar::Flag::Integer, "tone mapping operator", 0, 10);
CVAR(r_toneMapLuminanceLogMin, "-10", BE1::CVar::Flag::Float, "tone mapping luminance histogram minimum log2 luminance");
CVAR(r_toneMapLuminanceLogMax, "2", BE1::CVar::Flag::Float, "tone mapping luminance histogram maximum log2 luminance");
CVAR_MINMAX(r_toneMapLuminanceAdaptationRate, "1.1", BE1::CVar::Flag::Float, "tone mapping luminance adaptation rate", 0.0f, 100.0f);
CVAR_MINMAX(r_toneMapMiddleGray, "0.18", BE1::CVar::Flag::Float, "tone mapping middle gray", 0.0f, 10.0f);
CVAR_MINMAX(r_whiteBalanceTemperature, "6500", BE1::CVar::Flag::Float, "white balance color temperature in Kelvin", 1000.0f, 15000.0f);

CVAR(r_colorGrading, "0", BE1::CVar::Flag::Bool, "enable 3D LUT color grading");
CVAR_MINMAX(r_colorGradingSaturation, "1", BE1::CVar::Flag::Float, "color grading saturation", 0.0f, 2.0f);
CVAR_MINMAX(r_colorGradingContrast, "1", BE1::CVar::Flag::Float, "color grading contrast", 0.0f, 2.0f);
CVAR_MINMAX(r_colorGradingGamma, "1", BE1::CVar::Flag::Float, "color grading gamma", 0.1f, 4.0f);
CVAR_MINMAX(r_colorGradingGain, "1", BE1::CVar::Flag::Float, "color grading gain", 0.0f, 4.0f);
CVAR_MINMAX(r_colorGradingTintR, "1", BE1::CVar::Flag::Float, "color grading tint red multiplier", 0.0f, 2.0f);
CVAR_MINMAX(r_colorGradingTintG, "1", BE1::CVar::Flag::Float, "color grading tint green multiplier", 0.0f, 2.0f);
CVAR_MINMAX(r_colorGradingTintB, "1", BE1::CVar::Flag::Float, "color grading tint blue multiplier", 0.0f, 2.0f);

CVAR_MINMAX(r_lensDistortion, "0", BE1::CVar::Flag::Float, "lens distortion strength (positive = barrel, negative = pincushion)", -0.5f, 0.5f);
CVAR_MINMAX(r_chromaticAberration, "0", BE1::CVar::Flag::Float, "chromatic aberration channel offset at the screen edge in pixels", 0.0f, 32.0f);
CVAR_MINMAX(r_vignette, "0", BE1::CVar::Flag::Float, "vignette intensity", 0.0f, 1.0f);
CVAR(r_dithering, "1", BE1::CVar::Flag::Bool, "enable final output dithering");

CVAR_MINMAX(r_bloom, "2", BE1::CVar::Flag::Integer | BE1::CVar::Flag::Archive, "bloom mode (0 = off, 1 = pyramid, 2 = convolution)", RenderBloomMode::Off, RenderBloomMode::Convolution);
CVAR_MINMAX(r_bloomThreshold, "1.0", BE1::CVar::Flag::Float, "bloom luminance threshold", 0.0f, 100.0f);
CVAR_MINMAX(r_bloomSoftThreshold, "0.5", BE1::CVar::Flag::Float, "bloom soft threshold width", 0.0f, 10.0f);
CVAR_MINMAX(r_bloomIntensity, "0.15", BE1::CVar::Flag::Float, "final bloom composite intensity", 0.0f, 10.0f);
CVAR_MINMAX(r_bloomDirtIntensity, "0", BE1::CVar::Flag::Float | BE1::CVar::Flag::Archive, "bloom dirt mask intensity", 0.0f, 10.0f);

CVAR_MINMAX(r_pyramidBloomPrefilterQuality, "1", BE1::CVar::Flag::Integer, "pyramid bloom prefilter quality (0 = low quality, 1 = high quality)", 0, 1);
CVAR_MINMAX(r_pyramidBloomScatter, "0.67", BE1::CVar::Flag::Float, "pyramid bloom mip scatter weight", 0.0f, 1.0f);
CVAR_MINMAX(r_pyramidBloomPrefilterRadius, "1.25", BE1::CVar::Flag::Float, "pyramid bloom prefilter sample radius", 0.0f, 10.0f);
CVAR_MINMAX(r_pyramidBloomDownsampleRadius, "1.0", BE1::CVar::Flag::Float, "pyramid bloom downsample sample radius", 0.0f, 10.0f);
CVAR_MINMAX(r_pyramidBloomUpsampleRadius, "1.0", BE1::CVar::Flag::Float, "pyramid bloom upsample sample radius", 0.0f, 10.0f);
CVAR_MINMAX(r_pyramidBloomMipCount, "6", BE1::CVar::Flag::Integer, "pyramid bloom mip level count", 1, RenderBloomDownMaxLevelCount);

CVAR_MINMAX(r_convolutionBloomFFTMaxSize, "1024", BE1::CVar::Flag::Integer | BE1::CVar::Flag::Archive, "maximum convolution bloom FFT texture axis size", 256, 1024);
CVAR_MINMAX(r_convolutionBloomKernelSizeScale, "1.0", BE1::CVar::Flag::Float, "convolution bloom kernel spatial size scale", 0.01f, 10.0f);
CVAR_MINMAX(r_convolutionBloomKernelScale, "1.0", BE1::CVar::Flag::Float, "convolution bloom kernel spectrum scale", 0.0f, 10.0f);
CVAR(r_convolutionBloomKernelRotation, "0", BE1::CVar::Flag::Float, "convolution bloom kernel rotation in degrees");
CVAR_MINMAX(r_convolutionBloomPaddingTrim, "0.03", BE1::CVar::Flag::Float, "convolution bloom padding reduction as fraction of FFT size", 0.0f, 0.25f);
CVAR_MINMAX(r_convolutionBloomApertureScale, "0.25", BE1::CVar::Flag::Float, "convolution bloom aperture size within the FFT domain", 0.02f, 1.0f);
CVAR_MINMAX(r_convolutionBloomDiffractionPSFScale, "1.0", BE1::CVar::Flag::Float, "convolution bloom diffraction PSF intensity scale", 0.0f, 100.0f);
CVAR_MINMAX(r_convolutionBloomDispersionAmount, "1.0", BE1::CVar::Flag::Float, "convolution bloom diffraction dispersion amount", 0.0f, 10.0f);
