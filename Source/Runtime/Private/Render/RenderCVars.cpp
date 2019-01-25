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

CVAR(r_colorBits, "32", CVar::Integer | CVar::Archive, "number of color bits");
CVAR(r_depthBits, "24", CVar::Integer | CVar::Archive, "number of depth bits");
CVAR(r_stencilBits, "8", CVar::Integer | CVar::Archive, "number of stencil bits");
CVAR(r_multiSamples, "0", CVar::Integer | CVar::Archive, "number of antialiasing samples");

CVAR(r_gamma, "1.0", CVar::Float | CVar::Archive, "changes gamma tables");
CVAR(r_swapInterval, "0", CVar::Integer | CVar::Archive, "");
CVAR(r_dynamicVertexCacheSize, "0x800000", CVar::Integer, "size of dynamic vertex buffer");
CVAR(r_dynamicIndexCacheSize, "0x300000", CVar::Integer, "size of dynamic index buffer");
CVAR(r_dynamicUniformCacheSize, "0x200000", CVar::Integer, "size of dynamic uniform buffer");

CVAR(r_fastSkinning, "2", CVar::Integer | CVar::Archive, "matrix skinning calculation, 0 = CPU skinning, 1 = VS skinning, 2 = VTF skinning");
CVAR(r_vertexTextureUpdate, "2", CVar::Integer | CVar::Archive, "texel fetch buffer, 0 = direct copy, 1 = PBO, 2 = TBO");

CVAR(r_shadows, "1", CVar::Integer | CVar::Archive, "enable shadows, 1 = shadow map");
CVAR(r_shadowMapSize, "1024", CVar::Integer | CVar::Archive, "directional/projected shadow map size");
CVAR(r_shadowMapFilterSize, "1.0", CVar::Float | CVar::Archive, "shadow map blurring filter size in centi-meter unit");
CVAR(r_shadowCubeMapSize, "1024", CVar::Integer | CVar::Archive, "virtual shadow cube map size of each face");
CVAR(r_shadowCubeMapZNear, "0.04", CVar::Float, "");
CVAR(r_shadowCubeMapFloat, "0", CVar::Bool | CVar::Archive, "use float texture for shadow cube map");
CVAR(r_shadowMapQuality, "1", CVar::Integer | CVar::Archive, "shadow map PCF level, 0 = HW-PCF only, 1 = HW-PCFx5, 2 = PCFx9, 3 = PCFx16 (randomly jittered sample)");
CVAR(r_shadowMapCropAlign, "1", CVar::Bool, "");

CVAR(r_CSM_count, "4", CVar::Integer | CVar::Archive, "");
CVAR(r_CSM_maxDistance, "150", CVar::Float, "");
CVAR(r_CSM_pancaking, "0", CVar::Bool, "");
CVAR(r_CSM_nonCachedDistance, "50", CVar::Float, "");
CVAR(r_CSM_updateRatio, "1.0", CVar::Float, "");
CVAR(r_CSM_selectionMethod, "1", CVar::Integer, "cascade selection method, 0 = z-based selection, 1 = map-based selection");
CVAR(r_CSM_splitLamda, "0.9", CVar::Float, "");
CVAR(r_CSM_blend, "1", CVar::Bool, "");
CVAR(r_CSM_blendSize, "0.09375", CVar::Float, "");
CVAR(r_CSM_offsetFactor0, "3.0", CVar::Float, "scale value for CSM0 drawing");
CVAR(r_CSM_offsetFactor1, "2.5", CVar::Float, "scale value for CSM1 drawing");
CVAR(r_CSM_offsetFactor2, "2.0", CVar::Float, "scale value for CSM2 drawing");
CVAR(r_CSM_offsetFactor3, "1.5", CVar::Float, "scale value for CSM3 drawing");
CVAR(r_CSM_offsetUnits0, "16", CVar::Float, "bias value added to depth test for CSM0 drawing");
CVAR(r_CSM_offsetUnits1, "32", CVar::Float, "bias value added to depth test for CSM1 drawing");
CVAR(r_CSM_offsetUnits2, "64", CVar::Float, "bias value added to depth test for CSM2 drawing");
CVAR(r_CSM_offsetUnits3, "128", CVar::Float, "bias value added to depth test for CSM3 drawing");
CVAR(r_optimizedShadowProjection, "1", CVar::Integer, "");

CVAR(r_HOM, "0", CVar::Bool, "use hierarchical occlusion map culling");
CVAR(r_HOM_debug, "0", CVar::Bool, "");

CVAR(r_ambientLit, "1", CVar::Bool | CVar::Archive, "use ambient lighting");
CVAR(r_ambientScale, "0.5", CVar::Float | CVar::Archive, "ambient light intensities are mutipled by this");
CVAR(r_lightScale, "1.0", CVar::Float | CVar::Archive, "all light intensities are multiplied by this");
CVAR(r_offsetFactor, "1", CVar::Float, "polygon offset parameter");
CVAR(r_offsetUnits, "2", CVar::Float, "polygon offset parameter");
CVAR(r_queryWaitFrames, "10", CVar::Integer, "");
CVAR(r_instancing, "2", CVar::Integer | CVar::Archive, "");
CVAR(r_maxInstancingCount, "1024", CVar::Integer | CVar::Archive, "");

CVAR(r_HDR, "2", CVar::Integer | CVar::Archive, "HDR rendering type, 0 = no HDR, 1 = FP11 or FP16, 2 = FP16, 3 = FP32");
CVAR(r_HDR_debug, "0", CVar::Integer, "");
CVAR(r_HDR_bloomScale, "1.0", CVar::Float, "");
CVAR(r_HDR_toneMapping, "1", CVar::Bool, "");
CVAR(r_HDR_toneMapOp, "9", CVar::Integer, "0 = Linear, 1 = Exponential, 2 = Logarithmic, 3 = Drago Logarithmic, 4 = Reinhard, 5 = Reinhard Extended, 6 = Filmic ALU, 7 = Flimic ACES, 8 = Filmic Unreal, 9 = Filmic Uncharted 2");
CVAR(r_HDR_middleGray, "0.18", CVar::Float, "key value for tone mapping [0.045, 0.72]");
CVAR(r_HDR_brightLevel, "0.18", CVar::Float, "");
CVAR(r_HDR_brightMax, "0.8", CVar::Float, "");
CVAR(r_HDR_brightThreshold, "5.0", CVar::Float, "");
CVAR(r_HDR_brightOffset, "10.0", CVar::Float, "");

CVAR(r_sunShafts, "1", CVar::Bool | CVar::Archive, "sun shaft");
CVAR(r_sunShafts_scale, "16.0", CVar::Float, "sun shaft scale");

CVAR(r_SSAO, "1", CVar::Bool | CVar::Archive, "screen space ambient occlusion");
CVAR(r_SSAO_quality, "2", CVar::Integer | CVar::Archive, "");
CVAR(r_SSAO_blur, "1", CVar::Integer | CVar::Archive, "");
CVAR(r_SSAO_radius, "64.0", CVar::Float, "SSAO occlusion maximum ray radius in centi-meter unit");
CVAR(r_SSAO_validRange, "32.0", CVar::Float, "SSAO valid range in centi-meter unit");
CVAR(r_SSAO_threshold, "4.0", CVar::Float, "SSAO threshold in centi-meter unit");
CVAR(r_SSAO_power, "0.2", CVar::Float, "");
CVAR(r_SSAO_intensity, "1.0", CVar::Float, "");

CVAR(r_DOF, "0", CVar::Bool | CVar::Archive, "depth of field");
CVAR(r_DOF_cocScale, "1.0", CVar::Float, "");
CVAR(r_DOF_focalDistance, "100.0", CVar::Float, "");
CVAR(r_DOF_maxBlurNear, "10.0", CVar::Float, "");
CVAR(r_DOF_maxBlurFar, "1024.0", CVar::Float, "");
CVAR(r_DOF_noBlurFraction, "0.3", CVar::Float, "");

CVAR(r_motionBlur, "0", CVar::Integer | CVar::Archive, "motion blur, 0 = no blur, 1 = camera motion blur, 2 = object motion blur, 3 = camera & object motion blur");
CVAR(r_motionBlur_ShutterSpeed, "0.0167", CVar::Float, "camera shutter speed, default are 60hz");
CVAR(r_motionBlur_CameraAngleThreshold, "1.2", CVar::Float, "");
CVAR(r_motionBlur_Scale, "1.0", CVar::Float, "");
CVAR(r_motionBlur_MaxBlur, "0.024", CVar::Float, "");

CVAR(r_useDepthClamp, "1", CVar::Bool, "use depth clamp instead of no-far-clip-plane trick");
CVAR(r_useDepthBoundTest, "1", CVar::Bool, "use depth bounds test to reduce shadow fill");
CVAR(r_useDepthPrePass, "1", CVar::Bool | CVar::Archive, "");
CVAR(r_useDeferredLighting, "0", CVar::Bool | CVar::Archive, "");
CVAR(r_useTwoSidedStencil, "1", CVar::Bool, "do stencil shadows in one pass with different ops on each side");
CVAR(r_useLightScissors, "1", CVar::Bool, "use custom scissor rectangle for each light");
CVAR(r_useLightOcclusionQuery, "0", CVar::Bool, "");
CVAR(r_usePostProcessing, "1", CVar::Bool | CVar::Archive, "");

CVAR(r_skipBackEnd, "0", CVar::Bool, "don't draw anything");
CVAR(r_skipAmbientPass, "0", CVar::Bool, "skip ambient draw pass");
CVAR(r_skipShadowAndLitPass, "0", CVar::Bool, "skip shadow and lighting pass");
CVAR(r_skipFogLights, "0", CVar::Bool, "skip fog lights");
CVAR(r_skipBlendLights, "0", CVar::Bool, "skip blend lights");
CVAR(r_skipBlendPass, "0", CVar::Bool, "skip alpha blending draw pass");
CVAR(r_skipFinalPass, "0", CVar::Bool, "skip final draw pass");

CVAR(r_showStats, "0", CVar::Integer, "show rendering statistics");
CVAR(r_showBufferCache, "0", CVar::Bool, "print dynamic buffer usage every frame");
CVAR(r_showBufferCacheTiming, "1", CVar::Bool, "print dynamic buffer map/unmap timing every frame");
CVAR(r_showAABB, "0", CVar::Integer, "show axis-aligned bounding boxes");
CVAR(r_showWireframe, "0", CVar::Integer, "show wireframe");
CVAR(r_showSkeleton, "0", CVar::Integer, "show skeleton");
CVAR(r_showTextures, "0", CVar::Integer, "show textures");
CVAR(r_showRenderTarget, "0", CVar::Integer, "show render target texture");
CVAR(r_showRenderTarget_fullscreen, "0", CVar::Bool, "show render target texture fullscreen");
CVAR(r_showShadows, "0", CVar::Integer, "draw shadow visualization");
CVAR(r_showBloom, "0", CVar::Bool, "draw bloom color");
CVAR(r_showLights, "0", CVar::Integer, "");
CVAR(r_showLightScissorRects, "0", CVar::Integer, "");

CVAR(r_drawEntities, "1", CVar::Bool, "");
CVAR(r_noSubView, "0", CVar::Bool, "");
CVAR(r_subViewOnly, "0", CVar::Bool, "");
CVAR(r_singleTriangle, "0", CVar::Bool, "only draw a single triangle per primitive");

BE_NAMESPACE_END
