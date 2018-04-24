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
#include "RBackEnd.h"

BE_NAMESPACE_BEGIN

static void RB_BasePass(int numDrawSurfs, DrawSurf **drawSurfs, const VisibleLight *light) {
    const VisibleObject *prevSpace = nullptr;
    const SubMesh *     prevSubMesh = nullptr;
    const Material *    prevMaterial = nullptr;
    bool                prevDepthHack = false;

    backEnd.rbsurf.SetCurrentLight(light);

    for (int i = 0; i < numDrawSurfs; i++) {
        const DrawSurf *surf = drawSurfs[i];

        if (!(surf->flags & DrawSurf::AmbientVisible)) {
            continue;
        }

        if (!surf->material->IsLitSurface() && !surf->material->IsSkySurface()) {
            continue;
        }

        if (0) {//surf->material->HasRefraction()) {
            backEnd.ctx->updateCurrentRenderTexture = true;
            continue;
        }

        bool useInstancing = r_instancing.GetBool() && surf->material->GetPass()->instancingEnabled;

        bool isDifferentMaterial = surf->material != prevMaterial;
        bool isDifferentObject = surf->space != prevSpace;
        bool isDifferentSubMesh = prevSubMesh ? !surf->subMesh->IsShared(prevSubMesh) : true;
        bool isDifferentInstance = !useInstancing || !prevSpace || isDifferentMaterial || isDifferentSubMesh || prevSpace->def->state.flags != surf->space->def->state.flags || prevSpace->def->state.layer != surf->space->def->state.layer ? true : false;

        if (isDifferentMaterial || isDifferentObject) {
            if (prevMaterial && isDifferentInstance) {
                backEnd.rbsurf.Flush();
            }

            backEnd.rbsurf.Begin(surf->material->IsSkySurface() ? RBSurf::BackgroundFlush : RBSurf::AmbientFlush, surf->material, surf->materialRegisters, surf->space);

            prevSubMesh = surf->subMesh;
            prevMaterial = surf->material;

            if (isDifferentObject) {
                bool depthHack = !!(surf->space->def->state.flags & RenderObject::DepthHackFlag);

                if (prevDepthHack != depthHack) {
                    if (useInstancing) {
                        backEnd.rbsurf.Flush();
                    }

                    if (depthHack) {
                        rhi.SetDepthRange(0.0f, 0.1f);
                    } else {
                        rhi.SetDepthRange(0.0f, 1.0f);
                    }

                    prevDepthHack = depthHack;
                }

                if (!useInstancing) {
                    backEnd.modelViewMatrix = surf->space->modelViewMatrix;
                    backEnd.modelViewProjMatrix = surf->space->modelViewProjMatrix;
                }

                prevSpace = surf->space;
            }
        }

        if (useInstancing) {
            backEnd.rbsurf.AddInstance(surf);
        }

        backEnd.rbsurf.DrawSubMesh(surf->subMesh);
    }

    if (prevMaterial) {
        backEnd.rbsurf.Flush();
    }

    // restore depthHack
    if (prevDepthHack) {
        rhi.SetDepthRange(0.0f, 1.0f);
    }
}

void RB_ForwardBasePass(int numDrawSurfs, DrawSurf **drawSurfs) {
    if (backEnd.primaryLight) {
        RB_SetupLight(backEnd.primaryLight);

        if (r_shadows.GetInteger() != 0) {
            if ((backEnd.primaryLight->def->state.flags & RenderLight::CastShadowsFlag) && !(backEnd.view->def->state.flags & RenderView::NoShadows)) {
                RB_ShadowPass(backEnd.primaryLight);
            }
        }
    }

    // BaseLit or BaseAmbient or Sky
    RB_BasePass(numDrawSurfs, drawSurfs, backEnd.primaryLight);
}

BE_NAMESPACE_END
