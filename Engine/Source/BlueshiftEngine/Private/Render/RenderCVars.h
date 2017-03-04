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

#pragma once

#include "Core/CVars.h"

BE_NAMESPACE_BEGIN

/*
-------------------------------------------------------------------------------

    Render Console Variables

-------------------------------------------------------------------------------
*/

extern CVar     r_colorBits;
extern CVar     r_depthBits;
extern CVar     r_stencilBits;
extern CVar     r_multiSamples;

extern CVar     r_gamma;
extern CVar     r_swapInterval;
extern CVar     r_dynamicCacheVertexBytes;
extern CVar     r_dynamicCacheIndexBytes;

extern CVar     r_fastSkinning;
extern CVar     r_vertexTextureUpdate;

extern CVar     r_shadows;
extern CVar     r_shadowMapSize;
extern CVar     r_shadowMapFilterSize;
extern CVar     r_shadowMapFloat;
extern CVar     r_shadowCubeMapSize;
extern CVar     r_shadowCubeMapZNear;
extern CVar     r_shadowMapQuality;
extern CVar     r_shadowMapCropAlign;
extern CVar     r_CSM_count;
extern CVar     r_CSM_maxDistance;
extern CVar     r_CSM_pancaking;
extern CVar     r_CSM_nonCachedDistance;
extern CVar     r_CSM_updateRatio;
extern CVar     r_CSM_selectionMethod;
extern CVar     r_CSM_splitLamda;
extern CVar     r_CSM_blend;
extern CVar     r_CSM_blendSize;
extern CVar     r_CSM_offsetFactor0;
extern CVar     r_CSM_offsetFactor1;
extern CVar     r_CSM_offsetFactor2;
extern CVar     r_CSM_offsetFactor3;
extern CVar     r_CSM_offsetUnits0;
extern CVar     r_CSM_offsetUnits1;
extern CVar     r_CSM_offsetUnits2;
extern CVar     r_CSM_offsetUnits3;
extern CVar     r_optimizedShadowProjection;

extern CVar     r_HOM;
extern CVar     r_HOM_debug;

extern CVar     r_ambientLit;
extern CVar     r_ambientScale;
extern CVar     r_lightScale;
extern CVar     r_offsetFactor;
extern CVar     r_offsetUnits;
extern CVar     r_queryWaitFrames;

extern CVar     r_HDR;
extern CVar     r_HDR_debug;
extern CVar     r_HDR_middleGray;
extern CVar     r_HDR_brightLevel;
extern CVar     r_HDR_brightMax;
extern CVar     r_HDR_brightThrehold;
extern CVar     r_HDR_brightOffset;
extern CVar     r_HDR_bloomScale;

extern CVar     r_sunShafts;
extern CVar     r_sunShafts_scale;

extern CVar     r_SSAO;
extern CVar     r_SSAO_quality;
extern CVar     r_SSAO_blur;
extern CVar     r_SSAO_radius;
extern CVar     r_SSAO_validRange;
extern CVar     r_SSAO_threshold;
extern CVar     r_SSAO_power;
extern CVar     r_SSAO_intensity;

extern CVar     r_DOF;
extern CVar     r_DOF_cocScale;
extern CVar     r_DOF_focalDistance;
extern CVar     r_DOF_maxBlurNear;
extern CVar     r_DOF_maxBlurFar;
extern CVar     r_DOF_noBlurFraction;

extern CVar     r_motionBlur;
extern CVar     r_motionBlur_ShutterSpeed;
extern CVar     r_motionBlur_Scale;
extern CVar     r_motionBlur_MaxBlur;
extern CVar     r_motionBlur_CameraAngleThrehold;

extern CVar     r_useDepthClamp;
extern CVar     r_useDepthBoundTest;
extern CVar     r_useDepthPrePass;
extern CVar     r_useDeferredLighting;
extern CVar     r_useTwoSidedStencil;
extern CVar     r_useUnsmoothedTangents;
extern CVar     r_useLightScissors;
extern CVar     r_useLightOcclusionQuery;
extern CVar     r_usePostProcessing;

extern CVar     r_skipBackEnd;
extern CVar     r_skipAmbientPass;
extern CVar     r_skipShadowAndLitPass;
extern CVar     r_skipFogLights;
extern CVar     r_skipBlendPass;
extern CVar     r_skipFinalPass;

extern CVar     r_showStats;
extern CVar     r_showBufferCache;
extern CVar     r_showBufferCacheTiming;
extern CVar     r_showAABB;
extern CVar     r_showWireframe;
extern CVar     r_showSkeleton;
extern CVar     r_showTextures;
extern CVar     r_showRenderTarget;
extern CVar     r_showRenderTarget_fullscreen;
extern CVar     r_showShadows;
extern CVar     r_showBloom;
extern CVar     r_showLights;
extern CVar     r_showLightScissorRects;

extern CVar     r_drawEntities;
extern CVar     r_noSubView;
extern CVar     r_subViewOnly;

BE_NAMESPACE_END
