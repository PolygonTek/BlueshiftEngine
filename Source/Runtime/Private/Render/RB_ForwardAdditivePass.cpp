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

static void RB_LitPass(const VisibleLight *visLight) {
    const VisibleObject *prevSpace = nullptr;
    const SubMesh *     prevSubMesh = nullptr;
    const Material *    prevMaterial = nullptr;
    bool                prevDepthHack = false;
    Rect                prevScissorRect;

    backEnd.batch.SetCurrentLight(visLight);

    if (r_useLightScissors.GetBool()) {
        prevScissorRect = rhi.GetScissor();
        rhi.SetScissor(visLight->scissorRect);
    }

    if (r_useDepthBoundTest.GetBool()) {
        rhi.SetDepthBounds(backEnd.depthMin, backEnd.depthMax);
    }

    for (int i = 0; i < visLight->numDrawSurfs; i++) {
        const DrawSurf *surf = backEnd.drawSurfs[visLight->firstDrawSurf + i];

        if (!(surf->flags & DrawSurf::AmbientVisible)) {
            continue;
        }

        bool isDifferentObject = surf->space != prevSpace;
        bool isDifferentSubMesh = prevSubMesh ? !surf->subMesh->IsShared(prevSubMesh) : true;
        bool isDifferentMaterial = surf->material != prevMaterial;
        bool isDifferentInstance = !(surf->flags & DrawSurf::UseInstancing) || isDifferentMaterial || isDifferentSubMesh || !prevSpace || prevSpace->def->state.flags != surf->space->def->state.flags || prevSpace->def->state.layer != surf->space->def->state.layer ? true : false;

        if (isDifferentObject || isDifferentSubMesh || isDifferentMaterial) {
            if (prevMaterial && isDifferentInstance) {
                backEnd.batch.Flush();
            }

            backEnd.batch.Begin(Batch::LitFlush, surf->material, surf->materialRegisters, surf->space);

            prevSubMesh = surf->subMesh;
            prevMaterial = surf->material;

            if (isDifferentObject) {
                bool depthHack = !!(surf->space->def->state.flags & RenderObject::DepthHackFlag);

                if (prevDepthHack != depthHack) {
                    if (surf->flags & DrawSurf::UseInstancing) {
                        backEnd.batch.Flush();
                    }

                    if (depthHack) {
                        rhi.SetDepthRange(0.0f, 0.1f);
                    } else {
                        rhi.SetDepthRange(0.0f, 1.0f);
                    }

                    prevDepthHack = depthHack;
                }

                if (!(surf->flags & DrawSurf::UseInstancing)) {
                    backEnd.modelViewMatrix = surf->space->modelViewMatrix;
                    backEnd.modelViewProjMatrix = surf->space->modelViewProjMatrix;
                }

                prevSpace = surf->space;
            }
        }

        if (surf->flags & DrawSurf::UseInstancing) {
            backEnd.batch.AddInstance(surf);
        }

        backEnd.batch.DrawSubMesh(surf->subMesh);
    }

    if (prevMaterial) {
        backEnd.batch.Flush();
    }

    // restore depthHack
    if (prevDepthHack) {
        rhi.SetDepthRange(0.0f, 1.0f);
    }

    if (r_useDepthBoundTest.GetBool()) {
        rhi.SetDepthBounds(0.0f, 1.0f);
    }

    if (r_useLightScissors.GetBool()) {
        rhi.SetScissor(prevScissorRect);
    }
}

// Forward lighting renders each surfaces depending on lights that affect the surface.
void RB_ForwardAdditivePass(const LinkList<VisibleLight> *visLights) {
    for (VisibleLight *visLight = visLights->Next(); visLight; visLight = visLight->node.Next()) {
        const RenderLight *renderLight = visLight->def;

        if (renderLight->state.flags & RenderLight::PrimaryLightFlag) {
            continue;
        }

        if (r_useLightOcclusionQuery.GetBool() && !visLight->occlusionVisible) {
            continue;
        }

        if (r_useDepthBoundTest.GetBool()) {
            float lightDepthMin, lightDepthMax;

            if (!backEnd.view->def->GetDepthBoundsFromLight(renderLight, backEnd.view->def->viewProjMatrix, &lightDepthMin, &lightDepthMax)) {
                continue;
            }

            float visSurfDepthMin, visSurfDepthMax;

            if (backEnd.view->def->GetDepthBoundsFromAABB(visLight->litSurfsAABB, backEnd.view->def->viewProjMatrix, &visSurfDepthMin, &visSurfDepthMax)) {
                // FIXME:
                visSurfDepthMin = Max(visSurfDepthMin - 0.001, 0.0);
                visSurfDepthMax = Min(visSurfDepthMax + 0.001, 1.0);

                backEnd.depthMin = Max(lightDepthMin, visSurfDepthMin);
                backEnd.depthMax = Min(lightDepthMax, visSurfDepthMax);
            }
        }

        RB_SetupLight(visLight);

        if (r_shadows.GetInteger() != 0) {
            if ((visLight->def->state.flags & RenderLight::CastShadowsFlag) && !(backEnd.view->def->state.flags & RenderView::NoShadows)) {
                RB_ShadowPass(visLight);
            }
        }

        RB_LitPass(visLight);
    }
}

BE_NAMESPACE_END
