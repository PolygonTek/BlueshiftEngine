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

static void RB_LitPass(const viewLight_t *viewLight) {
    uint64_t            prevSortkey = -1;
    const viewEntity_t *prevSpace = nullptr;
    const Material *    prevMaterial = nullptr;
    bool                prevDepthHack = false;
    Rect                prevScissorRect;

    if (r_useLightScissors.GetBool()) {
        prevScissorRect = rhi.GetScissor();
        rhi.SetScissor(viewLight->scissorRect);
    }

    if (r_useDepthBoundTest.GetBool()) {
        rhi.SetDepthBounds(backEnd.depthMin, backEnd.depthMax);
    }

    for (drawSurfNode_t *litSurfNode = viewLight->litSurfs; litSurfNode; litSurfNode = litSurfNode->next) {
        const DrawSurf *surf = litSurfNode->drawSurf;

        if (!(surf->flags & DrawSurf::AmbientVisible)) {
            continue;
        }

        if (surf->sortKey != prevSortkey) {
            const Shader *shader = surf->material->GetPass()->shader;

            if (!shader) {
                continue;
            }

            if (!(shader->GetFlags() & Shader::LitSurface)) {
                continue;    
            }

            if (surf->material != prevMaterial || surf->space != prevSpace) {
                if (prevMaterial) {
                    backEnd.rbsurf.Flush();
                }

                backEnd.rbsurf.Begin(RBSurf::LitFlush, surf->material, surf->materialRegisters, surf->space, viewLight);

                prevMaterial = surf->material;
            }

            if (surf->space != prevSpace) {
                prevSpace = surf->space;

                backEnd.modelViewMatrix = surf->space->modelViewMatrix;
                backEnd.modelViewProjMatrix = surf->space->modelViewProjMatrix;

                bool depthHack = surf->space->def->parms.depthHack;

                if (prevDepthHack != depthHack) {
                    if (depthHack) {
                        rhi.SetDepthRange(0.0f, 0.1f);
                    } else {
                        rhi.SetDepthRange(0.0f, 1.0f);
                    }

                    prevDepthHack = depthHack;
                }
            }

            prevSortkey = surf->sortKey;
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

    if (r_useDepthBoundTest.GetBool()) {
        rhi.SetDepthBounds(0.0f, 1.0f);
    }

    if (r_useLightScissors.GetBool()) {
        rhi.SetScissor(prevScissorRect);
    }
}

void RB_ForwardAdditivePass(viewLight_t *viewLights) {
    for (viewLight_t *viewLight = viewLights; viewLight; viewLight = viewLight->next) {
        const SceneLight *light = viewLight->def;

        if (light->parms.isPrimaryLight) {
            continue;
        }

        if (r_useLightOcclusionQuery.GetBool() && !viewLight->occlusionVisible) {
            continue;
        }

        if (r_useDepthBoundTest.GetBool()) {
            float lightDepthMin, lightDepthMax;

            if (!backEnd.view->def->GetDepthBoundsFromLight(light, backEnd.view->def->viewProjMatrix, &lightDepthMin, &lightDepthMax)) {
                continue;
            }

            float visSurfDepthMin, visSurfDepthMax;

            if (backEnd.view->def->GetDepthBoundsFromAABB(viewLight->litSurfsAABB, backEnd.view->def->viewProjMatrix, &visSurfDepthMin, &visSurfDepthMax)) {
                // FIXME:
                visSurfDepthMin = Max(visSurfDepthMin - 0.001, 0.0);
                visSurfDepthMax = Min(visSurfDepthMax + 0.001, 1.0);

                backEnd.depthMin = Max(lightDepthMin, visSurfDepthMin);
                backEnd.depthMax = Min(lightDepthMax, visSurfDepthMax);
            }
        }

        RB_SetupLight(viewLight);

        if (r_shadows.GetInteger() != 0) {
            if (viewLight->def->parms.castShadows && !(backEnd.view->def->parms.flags & SceneView::NoShadows)) {
                RB_ShadowPass(viewLight);
            }
        }

        RB_LitPass(viewLight);
    }
}

BE_NAMESPACE_END
