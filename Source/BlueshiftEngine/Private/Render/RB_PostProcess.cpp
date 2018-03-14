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
#include "Render/Render.h"
#include "RenderInternal.h"

BE_NAMESPACE_BEGIN

void RB_PostProcessDepth() {
    if (r_SSAO.GetBool() || r_DOF.GetBool()) {
        PP_Downscale2x2(backEnd.ctx->screenRT->DepthStencilTexture(), backEnd.ctx->ppRTs[PP_RT_DEPTH_2X]);
        
        if (r_SSAO.GetBool()) {
            const Texture *depthMap2 = r_SSAO_quality.GetInteger() == 2 ? backEnd.ctx->screenRT->DepthStencilTexture() : backEnd.ctx->ppRTs[PP_RT_DEPTH_2X]->ColorTexture();
            PP_SSAO(backEnd.ctx->screenRT->DepthStencilTexture(), depthMap2, backEnd.view, backEnd.ctx->ppRTs[PP_RT_AO]);

            if (r_SSAO_blur.GetBool()) {
                //PP_LinearizeDepth(backEnd.ctx->screenRT->DepthStencilTexture(), backEnd.view->def->zNear, backEnd.view->def->zFar, backEnd.ctx->ppRTs[PP_RT_LINEAR_DEPTH]);
                PP_AoBlur(backEnd.ctx->ppRTs[PP_RT_AO]->ColorTexture(), backEnd.ctx->screenRT->DepthStencilTexture(), backEnd.ctx->ppRTs[PP_RT_AO_TEMP], backEnd.view->def->projMatrix, backEnd.ctx->ppRTs[PP_RT_AO]);
            }

            //backEnd.ctx->screenRT->Begin();
                        
            rhi.SetStateBits(RHI::ColorWrite | RHI::BS_DstColor | RHI::BD_Zero);
            rhi.SetCullFace(RHI::NoCull);

            const Shader *shader = ShaderManager::postPassThruShader;

            shader->Bind();
            shader->SetTexture("tex0", backEnd.ctx->ppRTs[PP_RT_AO]->ColorTexture());

            RB_DrawClipRect(0.0f, 0.0f, 1.0f, 1.0f);

            //backEnd.ctx->screenRT->End();
        }
    }
}

void RB_PostProcess() {
    float bloomScale;
    float screenTc[4];

    RenderContext *bc = backEnd.ctx;

    screenTc[0] = (float)backEnd.renderRect.x / bc->screenRT->GetWidth();
    screenTc[1] = (float)backEnd.renderRect.y / bc->screenRT->GetHeight();
    screenTc[2] = screenTc[0] + (float)backEnd.renderRect.w / bc->screenRT->GetWidth();
    screenTc[3] = screenTc[1] + (float)backEnd.renderRect.h / bc->screenRT->GetHeight();

    bool linearizeDepth = false;

    if (r_DOF.GetBool()) {
        PP_LinearizeDepth(bc->screenRT->DepthStencilTexture(), backEnd.view->def->zNear, backEnd.view->def->zFar, bc->ppRTs[PP_RT_LINEAR_DEPTH]);
        linearizeDepth = true;
    }

    if (r_sunShafts.GetBool() && backEnd.primaryLight) {
        const SceneLight *sunLight = backEnd.primaryLight->def;
        if (sunLight->parms.axis[0].Dot(backEnd.view->def->parms.axis[0]) < 0.0f) {
            if (!linearizeDepth) {
                linearizeDepth = true;
                PP_LinearizeDepth(bc->screenRT->DepthStencilTexture(), backEnd.view->def->zNear, backEnd.view->def->zFar, bc->ppRTs[PP_RT_LINEAR_DEPTH]);
            }

            PP_SunShaftsMaskGen(bc->screenRT->ColorTexture(), bc->ppRTs[PP_RT_LINEAR_DEPTH]->ColorTexture(), screenTc[0], screenTc[1], screenTc[2], screenTc[3], bc->ppRTs[PP_RT_2X]);			

            // shafts blur 8x -> 64x -> 512x
            Mat4 viewProjectionMatrix = backEnd.view->def->projMatrix * backEnd.view->def->viewMatrix.ToMat3().ToMat4();
            float invShaftsScale = 1.0f / r_sunShafts_scale.GetFloat();
            PP_SunShaftsGen(bc->ppRTs[PP_RT_2X]->ColorTexture(), viewProjectionMatrix, sunLight->parms.origin, invShaftsScale * 0.25f, bc->ppRTs[PP_RT_TEMP_2X]);
            PP_SunShaftsGen(bc->ppRTs[PP_RT_TEMP_2X]->ColorTexture(), viewProjectionMatrix, sunLight->parms.origin, invShaftsScale * 0.5f, bc->ppRTs[PP_RT_2X]);
            PP_SunShaftsGen(bc->ppRTs[PP_RT_2X]->ColorTexture(), viewProjectionMatrix, sunLight->parms.origin, invShaftsScale, bc->ppRTs[PP_RT_TEMP_2X]);			
        
            PP_PassThruPass(bc->screenRT->ColorTexture(), 0.0f, 0.0f, 1.0f, 1.0f, bc->ppRTs[PP_RT_TEMP]);

            Vec4 sunColor = Vec4(
                sunLight->parms.materialParms[SceneEntity::RedParm], 
                sunLight->parms.materialParms[SceneEntity::GreenParm], 
                sunLight->parms.materialParms[SceneEntity::BlueParm], 
                sunLight->parms.materialParms[SceneEntity::AlphaParm]);

            //sunColor *= sunLight->intensity;
            PP_SunShaftsDisplay(bc->ppRTs[PP_RT_TEMP]->ColorTexture(), bc->ppRTs[PP_RT_TEMP_2X]->ColorTexture(), sunColor, bc->screenRT);			
        }
    }

    if (r_motionBlur.GetInteger() & 2) {
        PP_PassThruPass(bc->screenRT->ColorTexture(), 0.0f, 0.0f, 1.0f, 1.0f, bc->ppRTs[PP_RT_TEMP]);

        PP_ObjectMotionBlur(bc->ppRTs[PP_RT_TEMP]->ColorTexture(), bc->ppRTs[PP_RT_VEL]->ColorTexture(), screenTc[0], screenTc[1], screenTc[2], screenTc[3], bc->screenRT);
    }

    if (r_DOF.GetBool()) {
        PP_Downscale2x2(bc->ppRTs[PP_RT_DEPTH_2X]->ColorTexture(), bc->ppRTs[PP_RT_DEPTH_4X]);

        PP_Downscale4x4(bc->screenRT->ColorTexture(), bc->ppRTs[PP_RT_4X]);
        
        PP_CopyCocToAlpha(bc->ppRTs[PP_RT_DEPTH_4X]->ColorTexture(), backEnd.view->def->zFar, bc->ppRTs[PP_RT_4X]);

        PP_CopyColorAndCoc(bc->screenRT->ColorTexture(), bc->ppRTs[PP_RT_LINEAR_DEPTH]->ColorTexture(), backEnd.view->def->zFar, bc->ppRTs[PP_RT_TEMP]);
        
        PP_GaussBlur7x7(bc->ppRTs[PP_RT_4X]->ColorTexture(), bc->ppRTs[PP_RT_TEMP_4X], bc->ppRTs[PP_RT_BLUR]);

        PP_ApplyDOF(bc->ppRTs[PP_RT_TEMP]->ColorTexture(), bc->ppRTs[PP_RT_BLUR]->ColorTexture(), bc->screenRT);
    }

    if (r_motionBlur.GetInteger() & 1) {
        Mat3 prevAxis;
        float minCos = 1.0f;

        for (int i = 0; i < 3; i++) {
            prevAxis[i] = backEnd.viewMatrixPrev[i].ToVec3();
            // 이전 프레임의 방향벡터의 각도 차이가 가장 큰 값의 cos 값을 구한다.
            float c = backEnd.view->def->viewMatrix[i].ToVec3().Dot(prevAxis[i]);
            if (c < minCos) {
                minCos = c;
            }
        }

        // camera motion blur 를 적용할지 결정
        float thresholdCos = Math::Cos(DEG2RAD(r_motionBlur_CameraAngleThrehold.GetFloat()));
        if (minCos < thresholdCos) {
            // 카메라의 rotation 에 대해서만 적용, translation 은 적용하지 않는다
            backEnd.viewMatrixPrev[0][3] = -(backEnd.view->def->parms.origin.Dot(prevAxis[0]));
            backEnd.viewMatrixPrev[1][3] = -(backEnd.view->def->parms.origin.Dot(prevAxis[1]));
            backEnd.viewMatrixPrev[2][3] = -(backEnd.view->def->parms.origin.Dot(prevAxis[2]));

            Mat4 viewProjMatrix[2];
            viewProjMatrix[0] = backEnd.view->def->viewProjMatrix;
            viewProjMatrix[1] = backEnd.view->def->projMatrix * backEnd.viewMatrixPrev;

            // 카메라 rotation 정도에 따라 iterative blur
            if (minCos > Math::Cos(DEG2RAD(4.0f))) {
                PP_PassThruPass(bc->screenRT->ColorTexture(), 0.0f, 0.0f, 1.0f, 1.0f, bc->ppRTs[PP_RT_TEMP]);
                PP_CameraMotionBlur(bc->ppRTs[PP_RT_TEMP]->ColorTexture(), bc->screenDepthTexture, viewProjMatrix, backEnd.view->def->parms.origin, 1.0f, bc->frameTime, bc->screenRT);
            } else if (minCos > Math::Cos(DEG2RAD(8.0f))) {
                PP_CameraMotionBlur(bc->screenRT->ColorTexture(), bc->screenDepthTexture, viewProjMatrix, backEnd.view->def->parms.origin, 1.0f, bc->frameTime, bc->ppRTs[PP_RT_TEMP]);
                PP_CameraMotionBlur(bc->ppRTs[PP_RT_TEMP]->ColorTexture(), bc->screenDepthTexture, viewProjMatrix, backEnd.view->def->parms.origin, 0.5f, bc->frameTime, bc->screenRT);
            } else {
                PP_PassThruPass(bc->screenRT->ColorTexture(), 0.0f, 0.0f, 1.0f, 1.0f, bc->ppRTs[PP_RT_TEMP]);
                PP_CameraMotionBlur(bc->ppRTs[PP_RT_TEMP]->ColorTexture(), bc->screenDepthTexture, viewProjMatrix, backEnd.view->def->parms.origin, 1.0f, bc->frameTime, bc->screenRT);
                PP_CameraMotionBlur(bc->screenRT->ColorTexture(), bc->screenDepthTexture, viewProjMatrix, backEnd.view->def->parms.origin, 0.5f, bc->frameTime, bc->ppRTs[PP_RT_TEMP]);
                PP_CameraMotionBlur(bc->ppRTs[PP_RT_TEMP]->ColorTexture(), bc->screenDepthTexture, viewProjMatrix, backEnd.view->def->parms.origin, 0.25f, bc->frameTime, bc->screenRT);
            }
        }
    }

    if (r_HDR.GetInteger() > 0) {
        // log luminance 값을 구한다
        PP_Downscale4x4(bc->screenRT->ColorTexture(), bc->ppRTs[PP_RT_4X]);
        PP_MeasureLuminance(bc->ppRTs[PP_RT_4X]->ColorTexture(), screenTc, bc->hdrLuminanceRT[0]);

        RenderTarget *luminanceRT;
        
        if (bc->flags & RenderContext::InstantToneMapping) {
            luminanceRT = bc->hdrLuminanceRT[0];
        } else {
            // 이전 프레임의 luminance 값을 이용해 luminance adaptation
            PP_LuminanceAdaptation(bc->hdrLuminanceRT[0]->ColorTexture(),
                bc->hdrLuminanceRT[bc->prevLumTarget]->ColorTexture(), bc->frameTime, bc->hdrLuminanceRT[bc->currLumTarget]);

            luminanceRT = bc->hdrLuminanceRT[bc->currLumTarget];

            Swap(bc->currLumTarget, bc->prevLumTarget);
        }

        if (r_HDR_bloomScale.GetFloat() > 0) {
            PP_BrightFilter(bc->ppRTs[PP_RT_4X]->ColorTexture(), luminanceRT->ColorTexture(), bc->hdrBloomRT[0]);
            PP_Downscale4x4(bc->hdrBloomRT[0]->ColorTexture(), bc->hdrBloomRT[1]);

            PP_KawaseBlur(bc->hdrBloomRT[0]->ColorTexture(), 0, bc->hdrBloomRT[1]);
            PP_KawaseBlur(bc->hdrBloomRT[1]->ColorTexture(), 1, bc->hdrBloomRT[0]);
            PP_KawaseBlur(bc->hdrBloomRT[0]->ColorTexture(), 2, bc->hdrBloomRT[1]);
            PP_KawaseBlur(bc->hdrBloomRT[1]->ColorTexture(), 3, bc->hdrBloomRT[0]);

            bloomScale = r_HDR_bloomScale.GetFloat();
        } else {
            bloomScale = 0.0f;
        }

        rhi.SetStateBits(RHI::ColorWrite | RHI::AlphaWrite);
        rhi.SetCullFace(RHI::NoCull);
        
        // HDR combine all = backBufferMap + bloomMap + luminanceMap

        const Shader *shader = ShaderManager::hdrFinalShader;

        shader->Bind();
        //shader->SetTexture("randomDir4x4Map", textureManager.randomDir4x4Texture);
        shader->SetTexture("colorMap", bc->screenRT->ColorTexture());
        shader->SetTexture("bloomMap0", bc->hdrBloomRT[0]->ColorTexture());
        shader->SetTexture("luminanceMap", luminanceRT->ColorTexture());
        shader->SetConstant1f("middleGray", r_HDR_middleGray.GetFloat());
        shader->SetConstant1f("bloomScale", bloomScale);
        shader->SetConstant1f("colorScale", r_showBloom.GetBool() ? 0.0f : 1.0f);
        //shader->SetConstant2f("frameRand", Vec2(renderConfig.random.RandomFloat(), renderConfig.random.RandomFloat()));

        RB_DrawClipRect(screenTc[0], screenTc[1], screenTc[2], screenTc[3]);
    } else {
        rhi.SetStateBits(RHI::ColorWrite | RHI::AlphaWrite);
        rhi.SetCullFace(RHI::NoCull);

        const Shader *shader = ShaderManager::postPassThruShader;

        shader->Bind();
        shader->SetTexture("tex0", bc->screenRT->ColorTexture());
        
        RB_DrawClipRect(screenTc[0], screenTc[1], screenTc[2], screenTc[3]);
    }
}

BE_NAMESPACE_END
