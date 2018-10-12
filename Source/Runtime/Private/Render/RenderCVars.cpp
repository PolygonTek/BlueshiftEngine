// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http ://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Precompiled.h"
#include "Core/CVars.h"

BE_NAMESPACE_BEGIN

CVAR(r_colorBits, L"32", CVar::Integer | CVar::Archive, L"number of color bits");
CVAR(r_depthBits, L"24", CVar::Integer | CVar::Archive, L"number of depth bits");
CVAR(r_stencilBits, L"8", CVar::Integer | CVar::Archive, L"number of stencil bits");
CVAR(r_multiSamples, L"0", CVar::Integer | CVar::Archive, L"number of antialiasing samples");

CVAR(r_gamma, L"1.0", CVar::Float | CVar::Archive, L"changes gamma tables");
CVAR(r_swapInterval, L"0", CVar::Integer | CVar::Archive, L"");
CVAR(r_dynamicVertexCacheSize, L"0x800000", CVar::Integer, L"size of dynamic vertex buffer");
CVAR(r_dynamicIndexCacheSize, L"0x300000", CVar::Integer, L"size of dynamic index buffer");
CVAR(r_dynamicUniformCacheSize, L"0x200000", CVar::Integer, L"size of dynamic uniform buffer");

CVAR(r_fastSkinning, L"2", CVar::Integer | CVar::Archive, L"matrix skinning calculation, 0 = CPU skinning, 1 = VS skinning, 2 = VTF skinning");
CVAR(r_vertexTextureUpdate, L"2", CVar::Integer | CVar::Archive, L"texel fetch buffer, 0 = direct copy, 1 = PBO, 2 = TBO");

CVAR(r_shadows, L"1", CVar::Integer | CVar::Archive, L"enable shadows, 1 = shadow map");
CVAR(r_shadowMapSize, L"1024", CVar::Integer | CVar::Archive, L"directional/projected shadow map size");
CVAR(r_shadowMapFilterSize, L"1.0", CVar::Float | CVar::Archive, L"shadow map blurring filter size in centi-meter unit");
CVAR(r_shadowCubeMapSize, L"1024", CVar::Integer | CVar::Archive, L"virtual shadow cube map size of each face");
CVAR(r_shadowCubeMapZNear, L"0.04", CVar::Float, L"");
CVAR(r_shadowCubeMapFloat, L"0", CVar::Bool | CVar::Archive, L"use float texture for shadow cube map");
CVAR(r_shadowMapQuality, L"1", CVar::Integer | CVar::Archive, L"shadow map PCF level, 0 = HW-PCF only, 1 = HW-PCFx5, 2 = PCFx9, 3 = PCFx16 (randomly jittered sample)");
CVAR(r_shadowMapCropAlign, L"1", CVar::Bool, L"");

CVAR(r_CSM_count, L"4", CVar::Integer | CVar::Archive, L"");
CVAR(r_CSM_maxDistance, L"100", CVar::Float, L"");
CVAR(r_CSM_pancaking, L"0", CVar::Bool, L"");
CVAR(r_CSM_nonCachedDistance, L"50", CVar::Float, L"");
CVAR(r_CSM_updateRatio, L"1.0", CVar::Float, L"");
CVAR(r_CSM_selectionMethod, L"1", CVar::Integer, L"cascade selection method, 0 = z-based selection, 1 = map-based selection");
CVAR(r_CSM_splitLamda, L"0.9", CVar::Float, L"");
CVAR(r_CSM_blend, L"1", CVar::Bool, L"");
CVAR(r_CSM_blendSize, L"0.09375", CVar::Float, L"");
CVAR(r_CSM_offsetFactor0, L"3.0", CVar::Float, L"scale value for CSM0 drawing");
CVAR(r_CSM_offsetFactor1, L"2.5", CVar::Float, L"scale value for CSM1 drawing");
CVAR(r_CSM_offsetFactor2, L"2.0", CVar::Float, L"scale value for CSM2 drawing");
CVAR(r_CSM_offsetFactor3, L"1.5", CVar::Float, L"scale value for CSM3 drawing");
CVAR(r_CSM_offsetUnits0, L"16", CVar::Float, L"bias value added to depth test for CSM0 drawing");
CVAR(r_CSM_offsetUnits1, L"32", CVar::Float, L"bias value added to depth test for CSM1 drawing");
CVAR(r_CSM_offsetUnits2, L"64", CVar::Float, L"bias value added to depth test for CSM2 drawing");
CVAR(r_CSM_offsetUnits3, L"128", CVar::Float, L"bias value added to depth test for CSM3 drawing");
CVAR(r_optimizedShadowProjection, L"1", CVar::Integer, L"");

CVAR(r_HOM, L"0", CVar::Bool, L"use hierarchical occlusion map culling");
CVAR(r_HOM_debug, L"0", CVar::Bool, L"");

CVAR(r_ambientLit, L"1", CVar::Bool | CVar::Archive, L"use ambient lighting");
CVAR(r_ambientScale, L"0.5", CVar::Float | CVar::Archive, L"ambient light intensities are mutipled by this");
CVAR(r_lightScale, L"1.0", CVar::Float | CVar::Archive, L"all light intensities are multiplied by this");
CVAR(r_offsetFactor, L"1", CVar::Float, L"polygon offset parameter");
CVAR(r_offsetUnits, L"2", CVar::Float, L"polygon offset parameter");
CVAR(r_queryWaitFrames, L"10", CVar::Integer, L"");
CVAR(r_instancing, L"2", CVar::Integer | CVar::Archive, L"");
CVAR(r_maxInstancingCount, L"1024", CVar::Integer | CVar::Archive, L"");

CVAR(r_HDR, L"2", CVar::Integer | CVar::Archive, L"HDR rendering type, 0 = no HDR, 1 = FP11 or FP16, 2 = FP16, 3 = FP32");
CVAR(r_HDR_debug, L"0", CVar::Integer, L"");
CVAR(r_HDR_bloomScale, L"1.0", CVar::Float, L"");
CVAR(r_HDR_middleGray, L"0.18", CVar::Float, L"");
CVAR(r_HDR_brightLevel, L"1.01", CVar::Float, L"");
CVAR(r_HDR_brightMax, L"4.5", CVar::Float, L"");
CVAR(r_HDR_brightThrehold, L"7.6", CVar::Float, L"");
CVAR(r_HDR_brightOffset, L"6.0", CVar::Float, L"");

CVAR(r_sunShafts, L"1", CVar::Bool | CVar::Archive, L"sun shaft");
CVAR(r_sunShafts_scale, L"16.0", CVar::Float, L"sun shaft scale");

CVAR(r_SSAO, L"1", CVar::Bool | CVar::Archive, L"screen space ambient occlusion");
CVAR(r_SSAO_quality, L"2", CVar::Integer | CVar::Archive, L"");
CVAR(r_SSAO_blur, L"1", CVar::Integer | CVar::Archive, L"");
CVAR(r_SSAO_radius, L"64.0", CVar::Float, L"SSAO occlusion maximum ray radius in centi-meter unit");
CVAR(r_SSAO_validRange, L"32.0", CVar::Float, L"SSAO valid range in centi-meter unit");
CVAR(r_SSAO_threshold, L"4.0", CVar::Float, L"SSAO threshold in centi-meter unit");
CVAR(r_SSAO_power, L"0.2", CVar::Float, L"");
CVAR(r_SSAO_intensity, L"1.0", CVar::Float, L"");

CVAR(r_DOF, L"0", CVar::Bool | CVar::Archive, L"depth of field");
CVAR(r_DOF_cocScale, L"1.0", CVar::Float, L"");
CVAR(r_DOF_focalDistance, L"100.0", CVar::Float, L"");
CVAR(r_DOF_maxBlurNear, L"10.0", CVar::Float, L"");
CVAR(r_DOF_maxBlurFar, L"1024.0", CVar::Float, L"");
CVAR(r_DOF_noBlurFraction, L"0.3", CVar::Float, L"");

CVAR(r_motionBlur, L"0", CVar::Integer | CVar::Archive, L"motion blur, 0 = no blur, 1 = camera motion blur, 2 = object motion blur, 3 = camera & object motion blur");
CVAR(r_motionBlur_ShutterSpeed, L"0.0167", CVar::Float, L"camera shutter speed, default are 60hz");
CVAR(r_motionBlur_CameraAngleThrehold, L"1.2", CVar::Float, L"");
CVAR(r_motionBlur_Scale, L"1.0", CVar::Float, L"");
CVAR(r_motionBlur_MaxBlur, L"0.024", CVar::Float, L"");

CVAR(r_useDepthClamp, L"1", CVar::Bool, L"use depth clamp instead of no-far-clip-plane trick");
CVAR(r_useDepthBoundTest, L"1", CVar::Bool, L"use depth bounds test to reduce shadow fill");
CVAR(r_useDepthPrePass, L"1", CVar::Bool | CVar::Archive, L"");
CVAR(r_useDeferredLighting, L"0", CVar::Bool | CVar::Archive, L"");
CVAR(r_useTwoSidedStencil, L"1", CVar::Bool, L"do stencil shadows in one pass with different ops on each side");
CVAR(r_useLightScissors, L"1", CVar::Bool, L"use custom scissor rectangle for each light");
CVAR(r_useLightOcclusionQuery, L"0", CVar::Bool, L"");
CVAR(r_usePostProcessing, L"1", CVar::Bool | CVar::Archive, L"");

CVAR(r_skipBackEnd, L"0", CVar::Bool, L"don't draw anything");
CVAR(r_skipAmbientPass, L"0", CVar::Bool, L"skip ambient draw pass");
CVAR(r_skipShadowAndLitPass, L"0", CVar::Bool, L"skip shadow and lighting pass");
CVAR(r_skipFogLights, L"0", CVar::Bool, L"skip fog lights");
CVAR(r_skipBlendLights, L"0", CVar::Bool, L"skip blend lights");
CVAR(r_skipBlendPass, L"0", CVar::Bool, L"skip alpha blending draw pass");
CVAR(r_skipFinalPass, L"0", CVar::Bool, L"skip final draw pass");

CVAR(r_showStats, L"0", CVar::Integer, L"show rendering statistics");
CVAR(r_showBufferCache, L"0", CVar::Bool, L"print dynamic buffer usage every frame");
CVAR(r_showBufferCacheTiming, L"1", CVar::Bool, L"print dynamic buffer map/unmap timing every frame");
CVAR(r_showAABB, L"0", CVar::Integer, L"show axis-aligned bounding boxes");
CVAR(r_showWireframe, L"0", CVar::Integer, L"show wireframe");
CVAR(r_showSkeleton, L"0", CVar::Integer, L"show skeleton");
CVAR(r_showTextures, L"0", CVar::Integer, L"show textures");
CVAR(r_showRenderTarget, L"0", CVar::Integer, L"show render target texture");
CVAR(r_showRenderTarget_fullscreen, L"0", CVar::Bool, L"show render target texture fullscreen");
CVAR(r_showShadows, L"0", CVar::Integer, L"draw shadow visualization");
CVAR(r_showBloom, L"0", CVar::Bool, L"draw bloom color");
CVAR(r_showLights, L"0", CVar::Integer, L"");
CVAR(r_showLightScissorRects, L"0", CVar::Integer, L"");

CVAR(r_drawEntities, L"1", CVar::Bool, L"");
CVAR(r_noSubView, L"0", CVar::Bool, L"");
CVAR(r_subViewOnly, L"0", CVar::Bool, L"");
CVAR(r_singleTriangle, L"0", CVar::Bool, L"only draw a single triangle per primitive");

BE_NAMESPACE_END
