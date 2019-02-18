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

static void RB_LitPass(const VisLight *visLight) {
    const VisObject *   prevSpace = nullptr;
    const SubMesh *     prevSubMesh = nullptr;
    const Material *    prevMaterial = nullptr;
    bool                prevDepthHack = false;
    Rect                prevScissorRect;
    bool                depthBoundTestEnabled = false;

    backEnd.batch.SetCurrentLight(visLight);

    if (r_useLightScissors.GetBool()) {
        prevScissorRect = rhi.GetScissor();
        rhi.SetScissor(visLight->scissorRect);
    }

    for (int i = 0; i < visLight->numDrawSurfs; i++) {
        const DrawSurf *drawSurf = backEnd.drawSurfs[visLight->firstDrawSurf + i];

        if (!(drawSurf->flags & DrawSurf::Visible)) {
            continue;
        }

        if (drawSurf->material->GetPass()->renderingMode != Material::RenderingMode::AlphaBlend || 
            drawSurf->material->GetPass()->transparency == Material::Transparency::TwoPassesOneSide) {
            if (!depthBoundTestEnabled && r_useDepthBoundTest.GetBool()) {
                depthBoundTestEnabled = true;
                rhi.SetDepthBounds(backEnd.depthMin, backEnd.depthMax);
            }
        } else {
            depthBoundTestEnabled = false;
            rhi.SetDepthBounds(0.0f, 1.0f);
        }

        bool isDifferentObject = drawSurf->space != prevSpace;
        bool isDifferentSubMesh = prevSubMesh ? !drawSurf->subMesh->IsShared(prevSubMesh) : true;
        bool isDifferentMaterial = drawSurf->material != prevMaterial;
        bool isDifferentInstance = !(drawSurf->flags & DrawSurf::UseInstancing) || isDifferentMaterial || isDifferentSubMesh || !prevSpace || 
            prevSpace->def->GetState().flags != drawSurf->space->def->GetState().flags || prevSpace->def->GetState().layer != drawSurf->space->def->GetState().layer ? true : false;

        if (isDifferentObject || isDifferentSubMesh || isDifferentMaterial) {
            if (prevMaterial && isDifferentInstance) {
                backEnd.batch.Flush();
            }

            backEnd.batch.Begin(Batch::LitFlush, drawSurf->material, drawSurf->materialRegisters, drawSurf->space);

            prevSubMesh = drawSurf->subMesh;
            prevMaterial = drawSurf->material;

            if (isDifferentObject) {
                bool depthHack = !!(drawSurf->space->def->GetState().flags & RenderObject::DepthHackFlag);

                if (prevDepthHack != depthHack) {
                    if (drawSurf->flags & DrawSurf::UseInstancing) {
                        backEnd.batch.Flush();
                    }

                    if (depthHack) {
                        rhi.SetDepthRange(0.0f, 0.1f);
                    } else {
                        rhi.SetDepthRange(0.0f, 1.0f);
                    }

                    prevDepthHack = depthHack;
                }

                if (!(drawSurf->flags & DrawSurf::UseInstancing)) {
                    backEnd.modelViewMatrix = drawSurf->space->modelViewMatrix;
                    backEnd.modelViewProjMatrix = drawSurf->space->modelViewProjMatrix;
                }

                prevSpace = drawSurf->space;
            }
        }

        if (drawSurf->flags & DrawSurf::UseInstancing) {
            backEnd.batch.AddInstance(drawSurf);
        }

        backEnd.batch.DrawSubMesh(drawSurf->subMesh);
    }

    // Flush previous batch
    if (prevMaterial) {
        backEnd.batch.Flush();
    }

    // Restore depth hack
    if (prevDepthHack) {
        rhi.SetDepthRange(0.0f, 1.0f);
    }

    if (depthBoundTestEnabled) {
        rhi.SetDepthBounds(0.0f, 1.0f);
    }

    if (r_useLightScissors.GetBool()) {
        rhi.SetScissor(prevScissorRect);
    }
}

// Forward lighting renders each surfaces depending on lights that affect the surface.
void RB_ForwardAdditivePass(const LinkList<VisLight> *visLights) {
    if (r_skipShadowAndLitPass.GetBool()) {
        return;
    }

    for (VisLight *visLight = visLights->Next(); visLight; visLight = visLight->node.Next()) {
        const RenderLight *renderLight = visLight->def;

        if (renderLight->GetState().flags & RenderLight::PrimaryLightFlag) {
            continue;
        }

        if (r_useLightOcclusionQuery.GetBool() && !visLight->occlusionVisible) {
            continue;
        }

        if (r_useDepthBoundTest.GetBool()) {
            float lightDepthMin, lightDepthMax;

            if (!backEnd.camera->def->CalcDepthBoundsFromLight(renderLight, backEnd.camera->def->GetViewProjMatrix(), &lightDepthMin, &lightDepthMax)) {
                continue;
            }

            float visSurfDepthMin, visSurfDepthMax;

            if (backEnd.camera->def->CalcDepthBoundsFromAABB(visLight->litSurfsAABB, backEnd.camera->def->GetViewProjMatrix(), &visSurfDepthMin, &visSurfDepthMax)) {
                // FIXME:
                visSurfDepthMin = Max(visSurfDepthMin - 0.001, 0.0);
                visSurfDepthMax = Min(visSurfDepthMax + 0.001, 1.0);

                backEnd.depthMin = Max(lightDepthMin, visSurfDepthMin);
                backEnd.depthMax = Min(lightDepthMax, visSurfDepthMax);
            }
        }

        RB_SetupLight(visLight);

        if (r_shadows.GetInteger() != 0) {
            if ((visLight->def->GetState().flags & RenderLight::CastShadowsFlag) && 
                !(backEnd.camera->def->GetState().flags & RenderCamera::NoShadows)) {
                RB_ShadowPass(visLight);
            }
        }

        RB_LitPass(visLight);
    }
}

BE_NAMESPACE_END
