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

CVAR(r_colorBits, "32", CVar::Flag::Integer | CVar::Flag::Archive, "number of color bits");
CVAR(r_depthBits, "24", CVar::Flag::Integer | CVar::Flag::Archive, "number of depth bits");
CVAR(r_stencilBits, "8", CVar::Flag::Integer | CVar::Flag::Archive, "number of stencil bits");
CVAR(r_multiSamples, "0", CVar::Flag::Integer | CVar::Flag::Archive, "number of antialiasing samples");

CVAR(r_sRGB, "1", CVar::Flag::Bool, "enable sRGB color calibration");
CVAR(r_gamma, "1.0", CVar::Flag::Float | CVar::Flag::Archive, "changes gamma tables");
CVAR(r_swapInterval, "-1", CVar::Flag::Integer | CVar::Flag::Archive, "control vsync, 0 = no vsync, 1 = vsync, -1 = adaptive vsync");
CVAR(r_dynamicVertexCacheSize, "0x800000", CVar::Flag::Integer, "size of dynamic vertex buffer");
CVAR(r_dynamicIndexCacheSize, "0x300000", CVar::Flag::Integer, "size of dynamic index buffer");
CVAR(r_dynamicUniformCacheSize, "0x200000", CVar::Flag::Integer, "size of dynamic uniform buffer");

CVAR(r_fastSkinning, "2", CVar::Flag::Integer | CVar::Flag::Archive, "matrix skinning calculation, 0 = CPU skinning, 1 = VS skinning, 2 = VTF skinning");
CVAR(r_vertexTextureUpdate, "2", CVar::Flag::Integer | CVar::Flag::Archive, "texel fetch buffer, 0 = direct copy, 1 = PBO, 2 = TBO");

CVAR(r_shadows, "1", CVar::Flag::Integer | CVar::Flag::Archive, "enable shadows, 1 = shadow map");
CVAR(r_shadowMapSize, "1024", CVar::Flag::Integer | CVar::Flag::Archive, "directional/projected shadow map size");
CVAR(r_shadowMapFilterSize, "1.0", CVar::Flag::Float | CVar::Flag::Archive, "shadow map blurring filter size in centi-meter unit");
CVAR(r_shadowCubeMapSize, "2048", CVar::Flag::Integer | CVar::Flag::Archive, "virtual shadow cube map size of each face");
CVAR(r_shadowCubeMapZNear, "0.04", CVar::Flag::Float, "");
CVAR(r_shadowCubeMapFloat, "0", CVar::Flag::Bool | CVar::Flag::Archive, "use float texture for shadow cube map");
CVAR(r_shadowMapQuality, "1", CVar::Flag::Integer | CVar::Flag::Archive, "shadow map PCF level, 0 = HW-PCF only, 1 = HW-PCFx5, 2 = PCFx9, 3 = PCFx16 (randomly jittered sample)");
CVAR(r_shadowMapCropAlign, "1", CVar::Flag::Bool, "");

CVAR(r_CSM_count, "4", CVar::Flag::Integer | CVar::Flag::Archive, "");
CVAR(r_CSM_maxDistance, "150", CVar::Flag::Float, "");
CVAR(r_CSM_pancaking, "0", CVar::Flag::Bool, "");
CVAR(r_CSM_nonCachedDistance, "50", CVar::Flag::Float, "");
CVAR(r_CSM_updateRatio, "1.0", CVar::Flag::Float, "");
CVAR(r_CSM_selectionMethod, "1", CVar::Flag::Integer, "cascade selection method, 0 = z-based selection, 1 = map-based selection");
CVAR(r_CSM_splitLambda, "0.9", CVar::Flag::Float, "");
CVAR(r_CSM_blend, "1", CVar::Flag::Bool, "");
CVAR(r_CSM_offsetFactor0, "5.0", CVar::Flag::Float, "scale value for CSM0 drawing");
CVAR(r_CSM_offsetFactor1, "4.0", CVar::Flag::Float, "scale value for CSM1 drawing");
CVAR(r_CSM_offsetFactor2, "3.0", CVar::Flag::Float, "scale value for CSM2 drawing");
CVAR(r_CSM_offsetFactor3, "2.0", CVar::Flag::Float, "scale value for CSM3 drawing");
CVAR(r_CSM_offsetUnits0, "1000", CVar::Flag::Float, "bias value added to depth test for CSM0 drawing");
CVAR(r_CSM_offsetUnits1, "500", CVar::Flag::Float, "bias value added to depth test for CSM1 drawing");
CVAR(r_CSM_offsetUnits2, "250", CVar::Flag::Float, "bias value added to depth test for CSM2 drawing");
CVAR(r_CSM_offsetUnits3, "125", CVar::Flag::Float, "bias value added to depth test for CSM3 drawing");
CVAR(r_optimizedShadowProjection, "1", CVar::Flag::Integer, "");

CVAR(r_HOM, "0", CVar::Flag::Bool, "use hierarchical occlusion map culling");
CVAR(r_HOM_debug, "0", CVar::Flag::Bool, "");

CVAR(r_ambientScale, "0.5", CVar::Flag::Float | CVar::Flag::Archive, "ambient intensities are mutipled by this");
CVAR(r_lightScale, "1.0", CVar::Flag::Float | CVar::Flag::Archive, "all light intensities are multiplied by this");
CVAR(r_indirectLit, "1", CVar::Flag::Bool | CVar::Flag::Archive, "use indirect lighting");
CVAR(r_specularEnergyCompensation, "0", CVar::Flag::Bool | CVar::Flag::Archive, "use energy compensation for multiple scattering in a microfacet model");

CVAR(r_probeBlending, "1", CVar::Flag::Bool | CVar::Flag::Archive, "use blending probe lighting");
CVAR(r_probeBoxProjection, "1", CVar::Flag::Bool | CVar::Flag::Archive, "use box projected cubemap");
CVAR_MINMAX(r_probeBakeBounces, "2", CVar::Flag::Integer | CVar::Flag::Archive, "", 1, 5);

CVAR(r_offsetFactor, "1", CVar::Flag::Float, "polygon offset parameter");
CVAR(r_offsetUnits, "2", CVar::Flag::Float, "polygon offset parameter");
CVAR(r_queryWaitFrames, "10", CVar::Flag::Integer, "");
CVAR(r_instancing, "2", CVar::Flag::Integer | CVar::Flag::Archive, "");
CVAR(r_maxInstancingCount, "1024", CVar::Flag::Integer | CVar::Flag::Archive, "");

CVAR(r_HDR, "2", CVar::Flag::Integer | CVar::Flag::Archive, "HDR rendering type, 0 = no HDR, 1 = FP11 or FP16, 2 = FP16, 3 = FP32");
CVAR(r_HDR_debug, "0", CVar::Flag::Integer, "");
CVAR(r_HDR_bloomScale, "1.0", CVar::Flag::Float, "");
CVAR(r_HDR_toneMapping, "1", CVar::Flag::Bool, "");
CVAR(r_HDR_toneMapOp, "9", CVar::Flag::Integer, "0 = Linear, 1 = Exponential, 2 = Logarithmic, 3 = Drago Logarithmic, 4 = Reinhard, 5 = Reinhard Extended, 6 = Filmic ALU, 7 = Flimic ACES, 8 = Filmic Unreal, 9 = Filmic Uncharted 2");
CVAR(r_HDR_middleGray, "0.18", CVar::Flag::Float, "key value for tone mapping [0.045, 0.72]");
CVAR(r_HDR_brightLevel, "0.18", CVar::Flag::Float, "");
CVAR(r_HDR_brightMax, "0.8", CVar::Flag::Float, "");
CVAR(r_HDR_brightThreshold, "5.0", CVar::Flag::Float, "");
CVAR(r_HDR_brightOffset, "10.0", CVar::Flag::Float, "");

CVAR(r_sunShafts, "1", CVar::Flag::Bool | CVar::Flag::Archive, "sun shaft");
CVAR(r_sunShafts_scale, "16.0", CVar::Flag::Float, "sun shaft scale");

CVAR(r_SSAO, "1", CVar::Flag::Bool | CVar::Flag::Archive, "screen space ambient occlusion");
CVAR(r_SSAO_quality, "2", CVar::Flag::Integer | CVar::Flag::Archive, "");
CVAR(r_SSAO_blur, "1", CVar::Flag::Integer | CVar::Flag::Archive, "");
CVAR(r_SSAO_radius, "64.0", CVar::Flag::Float, "SSAO occlusion maximum ray radius in centi-meter unit");
CVAR(r_SSAO_validRange, "32.0", CVar::Flag::Float, "SSAO valid range in centi-meter unit");
CVAR(r_SSAO_threshold, "4.0", CVar::Flag::Float, "SSAO threshold in centi-meter unit");
CVAR(r_SSAO_power, "0.2", CVar::Flag::Float, "");
CVAR(r_SSAO_intensity, "1.0", CVar::Flag::Float, "");

CVAR(r_DOF, "0", CVar::Flag::Bool | CVar::Flag::Archive, "depth of field");
CVAR(r_DOF_cocScale, "1.0", CVar::Flag::Float, "");
CVAR(r_DOF_focalDistance, "100.0", CVar::Flag::Float, "");
CVAR(r_DOF_maxBlurNear, "10.0", CVar::Flag::Float, "");
CVAR(r_DOF_maxBlurFar, "1024.0", CVar::Flag::Float, "");
CVAR(r_DOF_noBlurFraction, "0.3", CVar::Flag::Float, "");

CVAR(r_motionBlur, "0", CVar::Flag::Integer | CVar::Flag::Archive, "motion blur, 0 = no blur, 1 = camera motion blur, 2 = object motion blur, 3 = camera & object motion blur");
CVAR(r_motionBlur_ShutterSpeed, "0.0167", CVar::Flag::Float, "camera shutter speed, default are 60hz");
CVAR(r_motionBlur_CameraAngleThreshold, "1.2", CVar::Flag::Float, "");
CVAR(r_motionBlur_Scale, "1.0", CVar::Flag::Float, "");
CVAR(r_motionBlur_MaxBlur, "0.024", CVar::Flag::Float, "");

CVAR(r_useDepthClamp, "1", CVar::Flag::Bool, "use depth clamp instead of no-far-clip-plane trick");
CVAR(r_useDepthBoundTest, "1", CVar::Flag::Bool, "use depth bounds test to reduce shadow fill");
CVAR(r_useDepthPrePass, "1", CVar::Flag::Bool | CVar::Flag::Archive, "");
CVAR(r_useDeferredLighting, "0", CVar::Flag::Bool | CVar::Flag::Archive, "");
CVAR(r_useTwoSidedStencil, "1", CVar::Flag::Bool, "do stencil shadows in one pass with different ops on each side");
CVAR(r_useLightScissors, "1", CVar::Flag::Bool, "use custom scissor rectangle for each light");
CVAR(r_useLightOcclusionQuery, "0", CVar::Flag::Bool, "");
CVAR(r_usePostProcessing, "1", CVar::Flag::Bool | CVar::Flag::Archive, "");

CVAR(r_skipBackEnd, "0", CVar::Flag::Bool, "don't draw anything");
CVAR(r_skipBasePass, "0", CVar::Flag::Bool, "skip base draw pass");
CVAR(r_skipShadowAndLitPass, "0", CVar::Flag::Bool, "skip shadow and lighting pass");
CVAR(r_skipFogLights, "0", CVar::Flag::Bool, "skip fog lights");
CVAR(r_skipBlendLights, "0", CVar::Flag::Bool, "skip blend lights");
CVAR(r_skipBlendPass, "0", CVar::Flag::Bool, "skip alpha blending draw pass");
CVAR(r_skipFinalPass, "0", CVar::Flag::Bool, "skip final draw pass");

CVAR(r_showStats, "0", CVar::Flag::Integer, "show rendering statistics");
CVAR(r_showBufferCache, "0", CVar::Flag::Bool, "print dynamic buffer usage every frame");
CVAR(r_showBufferCacheTiming, "1", CVar::Flag::Bool, "print dynamic buffer map/unmap timing every frame");
CVAR(r_showAABB, "0", CVar::Flag::Integer, "show axis-aligned bounding boxes");
CVAR(r_showDynamicAABBTree, "0", CVar::Flag::Bool, "show dynamic AABB tree");
CVAR(r_showDynamicAABBTreeMinDepth, "0", CVar::Flag::Integer, "");
CVAR(r_showDynamicAABBTreeMaxDepth, "0", CVar::Flag::Integer, "");
CVAR(r_showWireframe, "0", CVar::Flag::Integer, "show wireframe");
CVAR(r_showSkeleton, "0", CVar::Flag::Integer, "show skeleton");
CVAR(r_showTextures, "0", CVar::Flag::Integer, "show textures");
CVAR(r_showRenderTarget, "0", CVar::Flag::Integer, "show render target texture");
CVAR(r_showRenderTarget_fullscreen, "0", CVar::Flag::Bool, "show render target texture fullscreen");
CVAR(r_showShadows, "0", CVar::Flag::Integer, "draw shadow visualization");
CVAR(r_showBloom, "0", CVar::Flag::Bool, "draw bloom color");
CVAR(r_showLights, "0", CVar::Flag::Integer, "");
CVAR(r_showLightScissorRects, "0", CVar::Flag::Integer, "");

CVAR(r_drawEntities, "1", CVar::Flag::Bool, "");
CVAR(r_noSubView, "0", CVar::Flag::Bool, "");
CVAR(r_subViewOnly, "0", CVar::Flag::Bool, "");
CVAR(r_singleTriangle, "0", CVar::Flag::Bool, "only draw a single triangle per primitive");

BE_NAMESPACE_END
