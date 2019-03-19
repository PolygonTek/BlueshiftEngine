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

static void RB_BasePass(int numDrawSurfs, DrawSurf **drawSurfs, const VisLight *light) {
    const VisObject *   prevSpace = nullptr;
    const SubMesh *     prevSubMesh = nullptr;
    const Material *    prevMaterial = nullptr;
    bool                prevDepthHack = false;

    backEnd.batch.SetCurrentLight(light);

    for (int i = 0; i < numDrawSurfs; i++) {
        const DrawSurf *drawSurf = drawSurfs[i];

        if (!drawSurf->material->IsLitSurface() && !drawSurf->material->IsSkySurface()) {
            continue;
        }

        if (0) {//surf->material->HasRefraction()) {
            backEnd.ctx->updateCurrentRenderTexture = true;
            continue;
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

            backEnd.batch.Begin(drawSurf->material->IsSkySurface() ? Batch::BackgroundFlush : Batch::BaseFlush, drawSurf->material, drawSurf->materialRegisters, drawSurf->space);

            prevSubMesh = drawSurf->subMesh;
            prevMaterial = drawSurf->material;

            if (isDifferentObject) {
                bool depthHack = !!(drawSurf->space->def->GetState().flags & RenderObject::Flag::DepthHack);

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
}

void RB_ForwardBasePass(int numDrawSurfs, DrawSurf **drawSurfs) {
    if (r_skipBasePass.GetBool()) {
        return;
    }

    if (backEnd.primaryLight) {
        RB_SetupLight(backEnd.primaryLight);

        if (r_shadows.GetInteger() != 0) {
            if ((backEnd.primaryLight->def->GetState().flags & RenderLight::Flag::CastShadows) &&
                !(backEnd.camera->def->GetState().flags & RenderCamera::Flag::NoShadows)) {
                RB_ShadowPass(backEnd.primaryLight);
            }
        }
    }

    RB_BasePass(numDrawSurfs, drawSurfs, backEnd.primaryLight);
}

BE_NAMESPACE_END
