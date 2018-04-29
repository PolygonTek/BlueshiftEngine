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

// Render sky only
void RB_BackgroundPass(int numDrawSurfs, DrawSurf **drawSurfs) {
    const VisibleObject *prevSpace = nullptr;
    const Material *    prevMaterial = nullptr;

    backEnd.batch.SetCurrentLight(nullptr);

    for (int i = 0; i < numDrawSurfs; i++) {
        const DrawSurf *surf = drawSurfs[i];
        if (!(surf->flags & DrawSurf::AmbientVisible)) {
            continue;
        }

        if (!surf->material->IsSkySurface()) {
            continue;
        }

        bool isDifferentObject = surf->space != prevSpace;
        bool isDifferentMaterial = surf->material != prevMaterial;

        if (isDifferentMaterial || isDifferentObject) {
            if (prevMaterial) {
                backEnd.batch.Flush();
            }

            backEnd.batch.Begin(Batch::BackgroundFlush, surf->material, surf->materialRegisters, surf->space);

            prevMaterial = surf->material;

            if (isDifferentObject) {
                prevSpace = surf->space;

                backEnd.modelViewMatrix = surf->space->modelViewMatrix;
                backEnd.modelViewProjMatrix = surf->space->modelViewProjMatrix;
            }
        }

        backEnd.batch.DrawSubMesh(surf->subMesh);
    }

    if (prevMaterial) {
        backEnd.batch.Flush();
    }
}

void RB_SelectionPass(int numDrawSurfs, DrawSurf **drawSurfs) {
    const VisibleObject *prevSpace = nullptr;
    const Material *    prevMaterial = nullptr;
    bool                prevDepthHack = false;

    backEnd.batch.SetCurrentLight(nullptr);

    for (int i = 0; i < numDrawSurfs; i++) {
        const DrawSurf *surf = drawSurfs[i];
        if ((surf->flags & DrawSurf::SkipSelection) || !(surf->flags & DrawSurf::AmbientVisible)) {
            continue;
        }

        if (surf->material->GetSort() == Material::Sort::SkySort) {
            continue;
        }

        bool isDifferentObject = surf->space != prevSpace;
        bool isDifferentMaterial = surf->material != prevMaterial;

        if (isDifferentMaterial || isDifferentObject) {
            if (surf->space->def->state.flags & RenderObject::SkipSelectionFlag) {
                continue;
            }

            if (!prevMaterial) {
                backEnd.batch.Begin(Batch::SelectionFlush, surf->material, surf->materialRegisters, surf->space);
            } else {
                if (isDifferentObject || prevMaterial->GetCullType() != surf->material->GetCullType() ||
                    (prevMaterial->GetSort() == Material::Sort::AlphaTestSort) || (surf->material->GetSort() == Material::Sort::AlphaTestSort)) {
                    backEnd.batch.Flush();
                    backEnd.batch.Begin(Batch::SelectionFlush, surf->material, surf->materialRegisters, surf->space);
                }
            }

            prevMaterial = surf->material;

            if (isDifferentObject) {
                bool depthHack = !!(surf->space->def->state.flags & RenderObject::DepthHackFlag);

                if (prevDepthHack != depthHack) {
                    if (depthHack) {
                        rhi.SetDepthRange(0.0f, 0.1f);
                    } else {
                        rhi.SetDepthRange(0.0f, 1.0f);
                    }

                    prevDepthHack = depthHack;
                }

                backEnd.modelViewMatrix = surf->space->modelViewMatrix;
                backEnd.modelViewProjMatrix = surf->space->modelViewProjMatrix;

                prevSpace = surf->space;
            }
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
}

void RB_OccluderPass(int numDrawSurfs, DrawSurf **drawSurfs) {
    const VisibleObject *prevSpace = nullptr;
    const Material *    prevMaterial = nullptr;
    bool                prevDepthHack = false;

    backEnd.batch.SetCurrentLight(nullptr);

    for (int i = 0; i < numDrawSurfs; i++) {
        const DrawSurf *surf = drawSurfs[i];
        if (!(surf->flags & DrawSurf::AmbientVisible)) {
            continue;
        }

        if (surf->material->GetSort() != Material::Sort::OpaqueSort) {
            continue;
        }

        bool isDifferentObject = surf->space != prevSpace;
        bool isDifferentMaterial = surf->material != prevMaterial;
            
        if (isDifferentMaterial || isDifferentObject) {
            if (!(surf->space->def->state.flags & RenderObject::OccluderFlag)) {
                continue;
            }

            /*if (surf->subMesh->GetAABB().Volume() < MeterToUnit(1) * MeterToUnit(1) * MeterToUnit(1)) {
                continue;
            }*/

            if (!prevMaterial) {
                backEnd.batch.Begin(Batch::OccluderFlush, surf->material, surf->materialRegisters, surf->space);
            } else {
                if (isDifferentObject) {
                    backEnd.batch.Flush();
                    backEnd.batch.Begin(Batch::OccluderFlush, surf->material, surf->materialRegisters, surf->space);
                }
            }

            prevMaterial = surf->material;

            if (isDifferentObject) {
                bool depthHack = !!(surf->space->def->state.flags & RenderObject::DepthHackFlag);

                if (prevDepthHack != depthHack) {
                    if (depthHack) {
                        rhi.SetDepthRange(0.0f, 0.1f);
                    } else {
                        rhi.SetDepthRange(0.0f, 1.0f);
                    }

                    prevDepthHack = depthHack;
                }

                backEnd.modelViewMatrix = surf->space->modelViewMatrix;
                backEnd.modelViewProjMatrix = surf->space->modelViewProjMatrix;

                prevSpace = surf->space;
            }
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
}

void RB_DepthPrePass(int numDrawSurfs, DrawSurf **drawSurfs) {
    const VisibleObject *prevSpace = nullptr;
    const SubMesh *     prevSubMesh = nullptr;
    const Material *    prevMaterial = nullptr;
    bool                prevDepthHack = false;

    backEnd.batch.SetCurrentLight(nullptr);

    for (int i = 0; i < numDrawSurfs; i++) {
        const DrawSurf *surf = drawSurfs[i];
        if (!(surf->flags & DrawSurf::AmbientVisible)) {
            continue;
        }

        if (!surf->material->IsLitSurface()) {
            continue;
        }

        if (surf->material->GetSort() != Material::Sort::OpaqueSort) {
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

            backEnd.batch.Begin(Batch::DepthFlush, surf->material, surf->materialRegisters, surf->space);

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
}

void RB_UnlitPass(int numDrawSurfs, DrawSurf **drawSurfs) {
    const VisibleObject *prevSpace = nullptr;
    const SubMesh *     prevSubMesh = nullptr;
    const Material *    prevMaterial = nullptr;
    bool                prevDepthHack = false;

    backEnd.batch.SetCurrentLight(nullptr);

    for (int i = 0; i < numDrawSurfs; i++) {
        const DrawSurf *surf = drawSurfs[i];
        if (!(surf->flags & DrawSurf::AmbientVisible)) {
            continue;
        }

        if (surf->material->IsLitSurface() || surf->material->IsSkySurface()) {
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

            backEnd.batch.Begin(Batch::UnlitFlush, surf->material, surf->materialRegisters, surf->space);

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
}

void RB_VelocityMapPass(int numDrawSurfs, DrawSurf **drawSurfs) {
    const VisibleObject *prevSpace = nullptr;
    const VisibleObject *skipObject = nullptr;
    const Material *    prevMaterial = nullptr;
    bool                firstDraw = true;

    backEnd.batch.SetCurrentLight(nullptr);

    for (int i = 0; i < numDrawSurfs; i++) {
        const DrawSurf *surf = drawSurfs[i];
        if (!(surf->flags & DrawSurf::AmbientVisible)) {
            continue;
        }

        if (surf->space == skipObject) {
            continue;
        }

        if (surf->material->GetSort() == Material::Sort::SkySort) {
            continue;
        }

        bool isDifferentObject = surf->space != prevSpace;
        bool isDifferentMaterial = surf->material != prevMaterial;
            
        if (isDifferentMaterial || isDifferentObject) {
            if (!prevMaterial) {
                backEnd.batch.Begin(Batch::VelocityFlush, surf->material, surf->materialRegisters, surf->space);
            } else {
                if (isDifferentObject || prevMaterial->GetCullType() != surf->material->GetCullType() ||
                    (prevMaterial->GetSort() == Material::Sort::AlphaTestSort) || (surf->material->GetSort() == Material::Sort::AlphaTestSort)) {
                    backEnd.batch.Flush();
                    backEnd.batch.Begin(Batch::VelocityFlush, surf->material, surf->materialRegisters, surf->space);
                }
            }

            prevMaterial = surf->material;

            if (isDifferentObject) {
                Mesh *mesh = surf->space->def->state.mesh;

                if (!mesh->IsSkinnedMesh() && (surf->space->def->worldMatrix == surf->space->def->prevWorldMatrix)) {
                    skipObject = surf->space;
                    continue;
                }

                skipObject = nullptr;

                backEnd.modelViewMatrix = surf->space->modelViewMatrix;
                backEnd.modelViewProjMatrix = surf->space->modelViewProjMatrix;

                prevSpace = surf->space;
            }
        }

        if (firstDraw) {
            firstDraw = false;

            backEnd.ctx->ppRTs[PP_RT_VEL]->Begin();

            rhi.SetViewport(Rect(0, 0, backEnd.ctx->ppRTs[PP_RT_VEL]->GetWidth(), backEnd.ctx->ppRTs[PP_RT_VEL]->GetHeight()));
            rhi.SetStateBits(RHI::ColorWrite | RHI::AlphaWrite | RHI::DepthWrite);
            rhi.Clear(RHI::ColorBit | RHI::DepthBit, Color4(0.0f, 0.0f, 0.0f, 1.0f), 1.0f, 0);
        }

        backEnd.batch.DrawSubMesh(surf->subMesh);
    }

    if (!firstDraw) {
        backEnd.batch.Flush();

        backEnd.ctx->ppRTs[PP_RT_VEL]->End();
        
        rhi.SetViewport(backEnd.renderRect);
        rhi.SetScissor(backEnd.renderRect);
    } else {
        firstDraw = false;

        backEnd.ctx->ppRTs[PP_RT_VEL]->Begin();

        rhi.SetViewport(Rect(0, 0, backEnd.ctx->ppRTs[PP_RT_VEL]->GetWidth(), backEnd.ctx->ppRTs[PP_RT_VEL]->GetHeight()));
        rhi.SetStateBits(RHI::ColorWrite | RHI::AlphaWrite | RHI::DepthWrite);
        rhi.Clear(RHI::ColorBit | RHI::DepthBit, Color4(0.0f, 0.0f, 0.0f, 1.0f), 1.0f, 0);
    
        backEnd.ctx->ppRTs[PP_RT_VEL]->End();

        rhi.SetViewport(backEnd.renderRect);
        rhi.SetScissor(backEnd.renderRect);
    }
}

void RB_FinalPass(int numDrawSurfs, DrawSurf **drawSurfs) {
    const VisibleObject *prevSpace = nullptr;
    const SubMesh *     prevSubMesh = nullptr;
    const Material *    prevMaterial = nullptr;
    bool                prevDepthHack = false;

    backEnd.batch.SetCurrentLight(nullptr);
    
    if (backEnd.ctx->updateCurrentRenderTexture) {
        backEnd.ctx->updateCurrentRenderTexture = false;
        backEnd.ctx->UpdateCurrentRenderTexture();
    }
        
    for (int i = 0; i < numDrawSurfs; i++) {
        const DrawSurf *surf = drawSurfs[i];
        if (!(surf->flags & DrawSurf::AmbientVisible)) {
            continue;
        }
        
        if (surf->material->GetSort() == Material::Sort::SkySort) {
            continue;
        }

        if (1) {//!surf->material->HasRefraction()) {
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

            backEnd.batch.Begin(Batch::FinalFlush, surf->material, surf->materialRegisters, surf->space);

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
}

void RB_GuiPass(int numDrawSurfs, DrawSurf **drawSurfs) {
    const VisibleObject *prevSpace = nullptr;
    const Material *    prevMaterial = nullptr;
    bool                prevDepthHack = false;

    backEnd.batch.SetCurrentLight(nullptr);
        
    for (int i = 0; i < numDrawSurfs; i++) {
        const DrawSurf *surf = drawSurfs[i];

        bool isDifferentObject = surf->space != prevSpace;
        bool isDifferentMaterial = surf->material != prevMaterial;

        if (isDifferentMaterial || isDifferentObject) {
            if (prevMaterial) {
                backEnd.batch.Flush();
            }

            backEnd.batch.Begin(Batch::GuiFlush, surf->material, surf->materialRegisters, surf->space);

            prevMaterial = surf->material;

            if (isDifferentObject) {
                bool depthHack = !!(surf->space->def->state.flags & RenderObject::DepthHackFlag);

                if (prevDepthHack != depthHack) {
                    if (depthHack) {
                        rhi.SetDepthRange(0.0f, 0.1f);
                    } else {
                        rhi.SetDepthRange(0.0f, 1.0f);
                    }

                    prevDepthHack = depthHack;
                }

                backEnd.modelViewMatrix = surf->space->modelViewMatrix;
                backEnd.modelViewProjMatrix = surf->space->modelViewProjMatrix;

                prevSpace = surf->space;
            }
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
}

BE_NAMESPACE_END
