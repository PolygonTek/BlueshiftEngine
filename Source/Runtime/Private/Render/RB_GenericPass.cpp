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
    uint64_t            prevSortkey = -1;
    const VisibleObject *prevSpace = nullptr;
    const Material *    prevMaterial = nullptr;
    bool                prevDepthHack = false;

    backEnd.rbsurf.SetCurrentLight(nullptr);

    for (int i = 0; i < numDrawSurfs; i++) {
        const DrawSurf *surf = drawSurfs[i];
        if (!(surf->flags & DrawSurf::AmbientVisible)) {
            continue;
        }

        if (surf->sortKey != prevSortkey) {
            if (!surf->material->IsSkySurface()) {
                continue;
            }

            bool isDifferentObject = surf->space != prevSpace;
            bool isDifferentMaterial = surf->material != prevMaterial;

            if (isDifferentMaterial || isDifferentObject) {
                if (prevMaterial) {
                    backEnd.rbsurf.Flush();
                }

                backEnd.rbsurf.Begin(RBSurf::BackgroundFlush, surf->material, surf->materialRegisters, surf->space);

                prevMaterial = surf->material;

                if (isDifferentObject) {
                    prevSpace = surf->space;

                    backEnd.modelViewMatrix = surf->space->modelViewMatrix;
                    backEnd.modelViewProjMatrix = surf->space->modelViewProjMatrix;
                }
            }

            prevSortkey = surf->sortKey;
        }

        backEnd.rbsurf.DrawSubMesh(surf->subMesh);
    }

    if (prevMaterial) {
        backEnd.rbsurf.Flush();
    }
}

void RB_SelectionPass(int numDrawSurfs, DrawSurf **drawSurfs) {
    uint64_t            prevSortkey = -1;
    const VisibleObject *prevSpace = nullptr;
    const Material *    prevMaterial = nullptr;
    bool                prevDepthHack = false;

    backEnd.rbsurf.SetCurrentLight(nullptr);

    for (int i = 0; i < numDrawSurfs; i++) {
        const DrawSurf *surf = drawSurfs[i];
        if ((surf->flags & DrawSurf::SkipSelection) || !(surf->flags & DrawSurf::AmbientVisible)) {
            continue;
        }

        if (surf->sortKey != prevSortkey) {
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
                    backEnd.rbsurf.Begin(RBSurf::SelectionFlush, surf->material, surf->materialRegisters, surf->space);
                } else {
                    if (isDifferentObject || prevMaterial->GetCullType() != surf->material->GetCullType() ||
                        (prevMaterial->GetSort() == Material::Sort::AlphaTestSort) || (surf->material->GetSort() == Material::Sort::AlphaTestSort)) {
                        backEnd.rbsurf.Flush();
                        backEnd.rbsurf.Begin(RBSurf::SelectionFlush, surf->material, surf->materialRegisters, surf->space);
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
}

void RB_OccluderPass(int numDrawSurfs, DrawSurf **drawSurfs) {
    uint64_t            prevSortkey = -1;
    const VisibleObject *prevSpace = nullptr;
    const Material *    prevMaterial = nullptr;
    bool                prevDepthHack = false;

    backEnd.rbsurf.SetCurrentLight(nullptr);

    for (int i = 0; i < numDrawSurfs; i++) {
        const DrawSurf *surf = drawSurfs[i];
        if (!(surf->flags & DrawSurf::AmbientVisible)) {
            continue;
        }

        if (surf->sortKey != prevSortkey) {
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
                    backEnd.rbsurf.Begin(RBSurf::OccluderFlush, surf->material, surf->materialRegisters, surf->space);
                } else {
                    if (isDifferentObject) {
                        backEnd.rbsurf.Flush();
                        backEnd.rbsurf.Begin(RBSurf::OccluderFlush, surf->material, surf->materialRegisters, surf->space);
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
}

void RB_DepthPrePass(int numDrawSurfs, DrawSurf **drawSurfs) {
    const VisibleObject *prevSpace = nullptr;
    const SubMesh *     prevSubMesh = nullptr;
    const Material *    prevMaterial = nullptr;
    bool                prevDepthHack = false;

    backEnd.rbsurf.SetCurrentLight(nullptr);

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

        bool useInstancing = r_instancing.GetBool() && surf->material->GetPass()->instancingEnabled;

        bool isDifferentMaterial = surf->material != prevMaterial;
        bool isDifferentObject = surf->space != prevSpace;
        bool isDifferentSubMesh = prevSubMesh ? !surf->subMesh->IsShared(prevSubMesh) : true;
        bool isDifferentInstance = !useInstancing || !prevSpace || isDifferentMaterial || isDifferentSubMesh || prevSpace->def->state.flags != surf->space->def->state.flags || prevSpace->def->state.layer != surf->space->def->state.layer ? true : false;

        if (isDifferentMaterial || isDifferentObject) {
            if (prevMaterial) {
                if ((isDifferentObject && isDifferentSubMesh) || 
                    prevMaterial->GetCullType() != surf->material->GetCullType()) {
                    backEnd.rbsurf.Flush();
                    backEnd.rbsurf.Begin(RBSurf::DepthFlush, surf->material, surf->materialRegisters, surf->space);
                }
            } else {
                backEnd.rbsurf.Begin(RBSurf::DepthFlush, surf->material, surf->materialRegisters, surf->space);
            }

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

void RB_UnlitPass(int numDrawSurfs, DrawSurf **drawSurfs) {
    const VisibleObject *prevSpace = nullptr;
    const SubMesh *     prevSubMesh = nullptr;
    const Material *    prevMaterial = nullptr;
    bool                prevDepthHack = false;

    backEnd.rbsurf.SetCurrentLight(nullptr);

    for (int i = 0; i < numDrawSurfs; i++) {
        const DrawSurf *surf = drawSurfs[i];
        if (!(surf->flags & DrawSurf::AmbientVisible)) {
            continue;
        }

        if (surf->material->IsLitSurface() || surf->material->IsSkySurface()) {
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

            backEnd.rbsurf.Begin(RBSurf::UnlitFlush, surf->material, surf->materialRegisters, surf->space);

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

void RB_VelocityMapPass(int numDrawSurfs, DrawSurf **drawSurfs) {
    uint64_t            prevSortkey = -1;
    const VisibleObject *prevSpace = nullptr;
    const VisibleObject *skipObject = nullptr;
    const Material *    prevMaterial = nullptr;
    bool                firstDraw = true;

    backEnd.rbsurf.SetCurrentLight(nullptr);

    for (int i = 0; i < numDrawSurfs; i++) {
        const DrawSurf *surf = drawSurfs[i];
        if (!(surf->flags & DrawSurf::AmbientVisible)) {
            continue;
        }

        if (surf->sortKey != prevSortkey) {
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
                    backEnd.rbsurf.Begin(RBSurf::VelocityFlush, surf->material, surf->materialRegisters, surf->space);
                } else {
                    if (isDifferentObject || prevMaterial->GetCullType() != surf->material->GetCullType() ||
                        (prevMaterial->GetSort() == Material::Sort::AlphaTestSort) || (surf->material->GetSort() == Material::Sort::AlphaTestSort)) {
                        backEnd.rbsurf.Flush();
                        backEnd.rbsurf.Begin(RBSurf::VelocityFlush, surf->material, surf->materialRegisters, surf->space);
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

            prevSortkey = surf->sortKey;
        }

        if (firstDraw) {
            firstDraw = false;

            backEnd.ctx->ppRTs[PP_RT_VEL]->Begin();

            rhi.SetViewport(Rect(0, 0, backEnd.ctx->ppRTs[PP_RT_VEL]->GetWidth(), backEnd.ctx->ppRTs[PP_RT_VEL]->GetHeight()));
            rhi.SetStateBits(RHI::ColorWrite | RHI::AlphaWrite | RHI::DepthWrite);
            rhi.Clear(RHI::ColorBit | RHI::DepthBit, Color4(0.0f, 0.0f, 0.0f, 1.0f), 1.0f, 0);
        }

        backEnd.rbsurf.DrawSubMesh(surf->subMesh);
    }

    if (!firstDraw) {
        backEnd.rbsurf.Flush();

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

    backEnd.rbsurf.SetCurrentLight(nullptr);
    
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

        bool useInstancing = r_instancing.GetBool() && surf->material->GetPass()->instancingEnabled;

        bool isDifferentMaterial = surf->material != prevMaterial;
        bool isDifferentObject = surf->space != prevSpace;
        bool isDifferentSubMesh = prevSubMesh ? !surf->subMesh->IsShared(prevSubMesh) : true;
        bool isDifferentInstance = !useInstancing || !prevSpace || isDifferentMaterial || isDifferentSubMesh || prevSpace->def->state.flags != surf->space->def->state.flags || prevSpace->def->state.layer != surf->space->def->state.layer ? true : false;

        if (isDifferentMaterial || isDifferentObject) {
            if (prevMaterial) {
                backEnd.rbsurf.Flush();
            }

            backEnd.rbsurf.Begin(RBSurf::FinalFlush, surf->material, surf->materialRegisters, surf->space);

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

void RB_GuiPass(int numDrawSurfs, DrawSurf **drawSurfs) {
    uint64_t            prevSortkey = -1;
    const VisibleObject *prevSpace = nullptr;
    const Material *    prevMaterial = nullptr;
    bool                prevDepthHack = false;

    backEnd.rbsurf.SetCurrentLight(nullptr);
        
    for (int i = 0; i < numDrawSurfs; i++) {
        const DrawSurf *surf = drawSurfs[i];
        if (surf->sortKey != prevSortkey) {
            bool isDifferentObject = surf->space != prevSpace;
            bool isDifferentMaterial = surf->material != prevMaterial;

            if (isDifferentMaterial || isDifferentObject) {
                if (prevMaterial) {
                    backEnd.rbsurf.Flush();
                }

                backEnd.rbsurf.Begin(RBSurf::GuiFlush, surf->material, surf->materialRegisters, surf->space);

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
}

BE_NAMESPACE_END
