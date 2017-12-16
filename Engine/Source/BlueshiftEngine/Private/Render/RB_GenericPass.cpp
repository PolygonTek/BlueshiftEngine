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

void RB_BackgroundPass(int numDrawSurfs, DrawSurf **drawSurfs) {
    uint64_t            prevSortkey = -1;
    const viewEntity_t *prevSpace = nullptr;
    const Material *    prevMaterial = nullptr;
    bool                prevDepthHack = false;

    for (int i = 0; i < numDrawSurfs; i++) {
        const DrawSurf *surf = drawSurfs[i];
        if (!(surf->flags & DrawSurf::AmbientVisible)) {
            continue;
        }

        if (surf->sortKey != prevSortkey) {
            const Shader *shader = surf->material->GetPass()->shader;

            if (!shader) {
                continue;
            }

            if (!(shader->GetFlags() & Shader::SkySurface)) {
                continue;
            }

            bool isDifferentEntity = surf->space != prevSpace ? true : false;
            bool isDifferentMaterial = surf->material != prevMaterial ? true : false;

            if (isDifferentMaterial || isDifferentEntity) {
                if (prevMaterial) {
                    backEnd.rbsurf.Flush();
                }

                backEnd.rbsurf.Begin(RBSurf::BackgroundFlush, surf->material, surf->materialRegisters, surf->space, backEnd.primaryLight);

                prevMaterial = surf->material;
            }

            if (isDifferentEntity) {
                prevSpace = surf->space;

                backEnd.modelViewMatrix = surf->space->modelViewMatrix;
                backEnd.modelViewProjMatrix = surf->space->modelViewProjMatrix;
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
    const viewEntity_t *prevSpace = nullptr;
    const Material *    prevMaterial = nullptr;
    bool                prevDepthHack = false;

    for (int i = 0; i < numDrawSurfs; i++) {
        const DrawSurf *surf = drawSurfs[i];
        if ((surf->flags & DrawSurf::SkipSelection) || !(surf->flags & DrawSurf::AmbientVisible)) {
            continue;
        }

        if (surf->sortKey != prevSortkey) {
            if (surf->material->GetSort() == Material::Sort::SkySort) {
                continue;
            }

            bool isDifferentEntity = surf->space != prevSpace ? true : false;
            bool isDifferentMaterial = surf->material != prevMaterial ? true : false;

            if (isDifferentMaterial || isDifferentEntity) {
                if (surf->space->def->parms.skipSelectionBuffer) {
                    continue;
                }

                if (!prevMaterial) {
                    backEnd.rbsurf.Begin(RBSurf::SelectionFlush, surf->material, surf->materialRegisters, surf->space, nullptr);
                } else {
                    if (isDifferentEntity || prevMaterial->GetCullType() != surf->material->GetCullType() ||
                        (prevMaterial->GetSort() == Material::Sort::AlphaTestSort) || (surf->material->GetSort() == Material::Sort::AlphaTestSort)) {
                        backEnd.rbsurf.Flush();
                        backEnd.rbsurf.Begin(RBSurf::SelectionFlush, surf->material, surf->materialRegisters, surf->space, nullptr);
                    }
                }

                prevMaterial = surf->material;
            }

            if (isDifferentEntity) {
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
}

void RB_OccluderPass(int numDrawSurfs, DrawSurf **drawSurfs) {
    uint64_t            prevSortkey = -1;
    const viewEntity_t *prevSpace = nullptr;
    const Material *    prevMaterial = nullptr;
    bool                prevDepthHack = false;

    for (int i = 0; i < numDrawSurfs; i++) {
        const DrawSurf *surf = drawSurfs[i];
        if (!(surf->flags & DrawSurf::AmbientVisible)) {
            continue;
        }

        if (surf->sortKey != prevSortkey) {
            if (surf->material->GetSort() != Material::Sort::OpaqueSort) {
                continue;
            }

            bool isDifferentEntity = surf->space != prevSpace ? true : false;
            bool isDifferentMaterial = surf->material != prevMaterial ? true : false;
            
            if (isDifferentMaterial || isDifferentEntity) {
                if (!surf->space->def->parms.occluder) {
                    continue;
                }

                /*if (surf->subMesh->GetAABB().Volume() < MeterToUnit(1) * MeterToUnit(1) * MeterToUnit(1)) {
                    continue;
                }*/
            
                if (!prevMaterial) {
                    backEnd.rbsurf.Begin(RBSurf::OccluderFlush, surf->material, surf->materialRegisters, surf->space, nullptr);
                } else {
                    if (isDifferentEntity) {
                        backEnd.rbsurf.Flush();
                        backEnd.rbsurf.Begin(RBSurf::OccluderFlush, surf->material, surf->materialRegisters, surf->space, nullptr);
                    }
                }

                prevMaterial = surf->material;
            }

            if (isDifferentEntity) {
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
}

void RB_DepthPrePass(int numDrawSurfs, DrawSurf **drawSurfs) {
    uint64_t            prevSortkey = -1;
    const viewEntity_t *prevSpace = nullptr;
    const Material *    prevMaterial = nullptr;
    bool                prevDepthHack = false;

    for (int i = 0; i < numDrawSurfs; i++) {
        const DrawSurf *surf = drawSurfs[i];
        if (!(surf->flags & DrawSurf::AmbientVisible)) {
            continue;
        }

        if (surf->sortKey != prevSortkey) {
            if (!surf->material->IsLitSurface()) {
                continue;
            }

            if (surf->material->GetSort() != Material::Sort::OpaqueSort) {
                continue;
            }

            bool isDifferentEntity = surf->space != prevSpace ? true : false;
            bool isDifferentMaterial = surf->material != prevMaterial ? true : false;

            if (isDifferentMaterial || isDifferentEntity) {
                if (!prevMaterial) {
                    backEnd.rbsurf.Begin(RBSurf::DepthFlush, surf->material, surf->materialRegisters, surf->space, nullptr);
                } else {
                    if (isDifferentEntity || prevMaterial->GetCullType() != surf->material->GetCullType()) {
                        backEnd.rbsurf.Flush();
                        backEnd.rbsurf.Begin(RBSurf::DepthFlush, surf->material, surf->materialRegisters, surf->space, nullptr);
                    }
                }

                prevMaterial = surf->material;
            }

            if (isDifferentEntity) {
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
}

void RB_UnlitPass(int numDrawSurfs, DrawSurf **drawSurfs) {
    uint64_t            prevSortkey = -1;
    const viewEntity_t *prevSpace = nullptr;
    const Material *    prevMaterial = nullptr;
    bool                prevDepthHack = false;

    for (int i = 0; i < numDrawSurfs; i++) {
        const DrawSurf *surf = drawSurfs[i];
        if (!(surf->flags & DrawSurf::AmbientVisible)) {
            continue;
        }

        if (surf->sortKey != prevSortkey) {
            const Shader *shader = surf->material->GetPass()->shader;
            if (shader) {
                if (shader->GetFlags() & (Shader::LitSurface | Shader::SkySurface)) {
                    continue;
                }
            }

            bool isDifferentEntity = surf->space != prevSpace ? true : false;
            bool isDifferentMaterial = surf->material != prevMaterial ? true : false;

            if (isDifferentMaterial || isDifferentEntity) {
                if (prevMaterial) {
                    backEnd.rbsurf.Flush();
                }

                backEnd.rbsurf.Begin(RBSurf::UnlitFlush, surf->material, surf->materialRegisters, surf->space, nullptr);

                prevMaterial = surf->material;
            }

            if (isDifferentEntity) {
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
}

void RB_VelocityMapPass(int numDrawSurfs, DrawSurf **drawSurfs) {
    uint64_t            prevSortkey = -1;
    const viewEntity_t *prevSpace = nullptr;
    const viewEntity_t *skipEntity = nullptr;
    const Material *    prevMaterial = nullptr;
    bool                firstDraw = true;

    for (int i = 0; i < numDrawSurfs; i++) {
        const DrawSurf *surf = drawSurfs[i];
        if (!(surf->flags & DrawSurf::AmbientVisible)) {
            continue;
        }

        if (surf->sortKey != prevSortkey) {
            if (surf->space == skipEntity) {
                continue;
            }

            if (surf->material->GetSort() == Material::Sort::SkySort) {
                continue;
            }

            bool isDifferentEntity = surf->space != prevSpace ? true : false;
            bool isDifferentMaterial = surf->material != prevMaterial ? true : false;
            
            if (isDifferentMaterial || isDifferentEntity) {
                if (!prevMaterial) {
                    backEnd.rbsurf.Begin(RBSurf::VelocityFlush, surf->material, surf->materialRegisters, surf->space, nullptr);
                } else {
                    if (isDifferentEntity || prevMaterial->GetCullType() != surf->material->GetCullType() || 
                        (prevMaterial->GetSort() == Material::Sort::AlphaTestSort) || (surf->material->GetSort() == Material::Sort::AlphaTestSort)) {
                        backEnd.rbsurf.Flush();
                        backEnd.rbsurf.Begin(RBSurf::VelocityFlush, surf->material, surf->materialRegisters, surf->space, nullptr);
                    }
                }

                prevMaterial = surf->material;
            }

            if (isDifferentEntity) {
                prevSpace = surf->space;

                Mesh *mesh = surf->space->def->parms.mesh;

                if (!mesh->IsSkinnedMesh() && (surf->space->def->motionBlurModelMatrix[0] == surf->space->def->motionBlurModelMatrix[1])) {
                    skipEntity = surf->space;
                    continue;
                }

                skipEntity = nullptr;

                backEnd.modelViewMatrix = surf->space->modelViewMatrix;
                backEnd.modelViewProjMatrix = surf->space->modelViewProjMatrix;
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
    uint64_t            prevSortkey = -1;
    const viewEntity_t *prevSpace = nullptr;
    const Material *    prevMaterial = nullptr;
    bool                prevDepthHack = false;
    
    if (backEnd.ctx->updateCurrentRenderTexture) {
        backEnd.ctx->updateCurrentRenderTexture = false;
        backEnd.ctx->UpdateCurrentRenderTexture();
    }
        
    for (int i = 0; i < numDrawSurfs; i++) {
        const DrawSurf *surf = drawSurfs[i];
        if (!(surf->flags & DrawSurf::AmbientVisible)) {
            continue;
        }
        
        if (surf->sortKey != prevSortkey) {
            if (surf->material->GetSort() == Material::Sort::SkySort) {
                continue;
            }

            if (1) {//!surf->material->HasRefraction()) {
                continue;
            }

            bool isDifferentEntity = surf->space != prevSpace ? true : false;
            bool isDifferentMaterial = surf->material != prevMaterial ? true : false;

            if (isDifferentMaterial || isDifferentEntity) {
                if (prevMaterial) {
                    backEnd.rbsurf.Flush();
                }

                backEnd.rbsurf.Begin(RBSurf::FinalFlush, surf->material, surf->materialRegisters, surf->space, nullptr);

                prevMaterial = surf->material;
            }

            if (isDifferentEntity) {
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
}

void RB_GuiPass(int numDrawSurfs, DrawSurf **drawSurfs) {
    uint64_t            prevSortkey = -1;
    const viewEntity_t *prevSpace = nullptr;
    const Material *    prevMaterial = nullptr;
    bool                prevDepthHack = false;
        
    for (int i = 0; i < numDrawSurfs; i++) {
        const DrawSurf *surf = drawSurfs[i];
        if (surf->sortKey != prevSortkey) {
            bool isDifferentEntity = surf->space != prevSpace ? true : false;
            bool isDifferentMaterial = surf->material != prevMaterial ? true : false;

            if (isDifferentMaterial || isDifferentEntity) {
                if (prevMaterial) {
                    backEnd.rbsurf.Flush();
                }

                backEnd.rbsurf.Begin(RBSurf::GuiFlush, surf->material, surf->materialRegisters, surf->space, nullptr);

                prevMaterial = surf->material;
            }

            if (isDifferentEntity) {
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
}

BE_NAMESPACE_END
