#include "Precompiled.h"
#include "Render/Render.h"
#include "RenderInternal.h"

BE_NAMESPACE_BEGIN

void RB_SelectionPass(int numDrawSurfs, DrawSurf **drawSurfs) {
    int                 prevSortkey = -1;
    viewEntity_t *      prevEntity = nullptr;
    const Material *    prevMaterial = nullptr;
    bool                prevDepthHack = false;

    for (int i = 0; i < numDrawSurfs; i++) {
        const DrawSurf *surf = drawSurfs[i];
        if (!(surf->flags & DrawSurf::AmbientVisible)) {
            continue;
        }

        if (surf->sortkey != prevSortkey) {
            if (surf->material->GetCoverage() == Material::EmptyCoverage) {
                continue;
            }            

            bool isDifferentEntity = surf->entity != prevEntity ? true : false;
            bool isDifferentMaterial = surf->material != prevMaterial ? true : false;

            if (isDifferentMaterial || isDifferentEntity) {
                if (surf->entity->def->parms.skipSelectionBuffer) {
                    continue;
                }

                if (!prevMaterial) {
                    backEnd.rbsurf.Begin(RBSurf::SelectionFlush, surf->material, surf->materialRegisters, surf->entity, nullptr);
                } else {
                    if (isDifferentEntity || prevMaterial->GetCullType() != surf->material->GetCullType() ||
                        (prevMaterial->GetCoverage() & Material::PerforatedCoverage) || (surf->material->GetCoverage() & Material::PerforatedCoverage)) {
                        backEnd.rbsurf.Flush();
                        backEnd.rbsurf.Begin(RBSurf::SelectionFlush, surf->material, surf->materialRegisters, surf->entity, nullptr);
                    }
                }

                prevMaterial = surf->material;
            }

            if (isDifferentEntity) {
                prevEntity = surf->entity;

                backEnd.modelViewMatrix = surf->entity->modelViewMatrix;
                backEnd.modelViewProjMatrix = surf->entity->modelViewProjMatrix;

                bool depthHack = surf->entity->def->parms.depthHack;
            
                if (prevDepthHack != depthHack) {
                    if (depthHack) {
                        glr.SetDepthRange(0.0f, 0.1f);
                    } else {
                        glr.SetDepthRange(0.0f, 1.0f);
                    }

                    prevDepthHack = depthHack;
                }
            }

            prevSortkey = surf->sortkey;
        }

        backEnd.rbsurf.DrawSubMesh(surf->subMesh, surf->guiSubMesh);
    }

    if (prevMaterial) {
        backEnd.rbsurf.Flush();
    }
    
    // restore depthHack
    if (prevDepthHack) {
        glr.SetDepthRange(0.0f, 1.0f);
    }
}

void RB_OccluderPass(int numDrawSurfs, DrawSurf **drawSurfs) {
    int                 prevSortkey = -1;
    viewEntity_t *      prevEntity = nullptr;
    const Material *    prevMaterial = nullptr;
    bool                prevDepthHack = false;

    for (int i = 0; i < numDrawSurfs; i++) {
        const DrawSurf *surf = drawSurfs[i];
        if (!(surf->flags & DrawSurf::AmbientVisible)) {
            continue;
        }

        if (surf->sortkey != prevSortkey) {
            if (!(surf->material->GetCoverage() & Material::OpaqueCoverage)) {
                continue;
            }

            bool isDifferentEntity = surf->entity != prevEntity ? true : false;
            bool isDifferentMaterial = surf->material != prevMaterial ? true : false;
            
            if (isDifferentMaterial || isDifferentEntity) {
                if (!surf->entity->def->parms.occluder) {
                    continue;
                }

                /*if (surf->subMesh->GetAABB().Volume() < MeterToUnit(1) * MeterToUnit(1) * MeterToUnit(1)) {
                    continue;
                }*/
            
                if (!prevMaterial) {
                    backEnd.rbsurf.Begin(RBSurf::OccluderFlush, surf->material, surf->materialRegisters, surf->entity, nullptr);
                } else {
                    if (isDifferentEntity) {
                        backEnd.rbsurf.Flush();
                        backEnd.rbsurf.Begin(RBSurf::OccluderFlush, surf->material, surf->materialRegisters, surf->entity, nullptr);
                    }
                }

                prevMaterial = surf->material;
            }

            if (isDifferentEntity) {
                prevEntity = surf->entity;

                backEnd.modelViewMatrix = surf->entity->modelViewMatrix;
                backEnd.modelViewProjMatrix = surf->entity->modelViewProjMatrix;

                bool depthHack = surf->entity->def->parms.depthHack;
            
                if (prevDepthHack != depthHack) {
                    if (depthHack) {
                        glr.SetDepthRange(0.0f, 0.1f);
                    } else {
                        glr.SetDepthRange(0.0f, 1.0f);
                    }

                    prevDepthHack = depthHack;
                }				
            }

            prevSortkey = surf->sortkey;
        }

        backEnd.rbsurf.DrawSubMesh(surf->subMesh, surf->guiSubMesh);
    }

    if (prevMaterial) {
        backEnd.rbsurf.Flush();
    }

    // restore depthHack
    if (prevDepthHack) {
        glr.SetDepthRange(0.0f, 1.0f);
    }
}

void RB_DepthPrePass(int numDrawSurfs, DrawSurf **drawSurfs) {
    int                 prevSortkey = -1;
    viewEntity_t *      prevEntity = nullptr;
    const Material *    prevMaterial = nullptr;
    bool                prevDepthHack = false;

    for (int i = 0; i < numDrawSurfs; i++) {
        const DrawSurf *surf = drawSurfs[i];
        if (!(surf->flags & DrawSurf::AmbientVisible)) {
            continue;
        }

        if (surf->sortkey != prevSortkey) {
            if (!(surf->material->GetCoverage() & (Material::OpaqueCoverage | Material::PerforatedCoverage))) {
                continue;
            }

            /*if (surf->material->HasRefraction()) {
                backEnd.ctx->updateCurrentRenderTexture = true;
                continue;
            }*/

            bool isDifferentEntity = surf->entity != prevEntity ? true : false;
            bool isDifferentMaterial = surf->material != prevMaterial ? true : false;

            if (isDifferentMaterial || isDifferentEntity) {
                if (!prevMaterial) {
                    backEnd.rbsurf.Begin(RBSurf::DepthFlush, surf->material, surf->materialRegisters, surf->entity, nullptr);
                } else {
                    // NOTE: alpha test 가 early-z culling 을 disable 하지 않는지 테스트 필요
                    if (isDifferentEntity || prevMaterial->GetCullType() != surf->material->GetCullType() ||
                        (prevMaterial->GetCoverage() & Material::PerforatedCoverage) || (surf->material->GetCoverage() & Material::PerforatedCoverage)) {
                        backEnd.rbsurf.Flush();
                        backEnd.rbsurf.Begin(RBSurf::DepthFlush, surf->material, surf->materialRegisters, surf->entity, nullptr);
                    }
                }

                prevMaterial = surf->material;
            }

            if (isDifferentEntity) {
                prevEntity = surf->entity;

                backEnd.modelViewMatrix = surf->entity->modelViewMatrix;
                backEnd.modelViewProjMatrix = surf->entity->modelViewProjMatrix;

                bool depthHack = surf->entity->def->parms.depthHack;
            
                if (prevDepthHack != depthHack) {
                    if (depthHack) {
                        glr.SetDepthRange(0.0f, 0.1f);
                    } else {
                        glr.SetDepthRange(0.0f, 1.0f);
                    }

                    prevDepthHack = depthHack;
                }				
            }

            prevSortkey = surf->sortkey;
        }

        backEnd.rbsurf.DrawSubMesh(surf->subMesh, surf->guiSubMesh);
    }

    if (prevMaterial) {
        backEnd.rbsurf.Flush();
    }
    
    // restore depthHack
    if (prevDepthHack) {
        glr.SetDepthRange(0.0f, 1.0f);
    }
}

void RB_AmbientPass(int numDrawSurfs, DrawSurf **drawSurfs) {
    int                 prevSortkey = -1;
    viewEntity_t *      prevEntity = nullptr;
    const Material *    prevMaterial = nullptr;
    bool                prevDepthHack = false;
        
    for (int i = 0; i < numDrawSurfs; i++) {
        const DrawSurf *surf = drawSurfs[i];
        if (!(surf->flags & DrawSurf::AmbientVisible)) {
            continue;
        }

        if (surf->sortkey != prevSortkey) {
            if (!(surf->material->GetCoverage() & (Material::OpaqueCoverage | Material::PerforatedCoverage))) {
                continue;
            }

            if (0) {//surf->material->HasRefraction()) {
                backEnd.ctx->updateCurrentRenderTexture = true;
                continue;
            }

            bool isDifferentEntity = surf->entity != prevEntity ? true : false;
            bool isDifferentMaterial = surf->material != prevMaterial ? true : false;

            if (isDifferentMaterial || isDifferentEntity) {
                if (prevMaterial) {
                    backEnd.rbsurf.Flush();
                }
                backEnd.rbsurf.Begin(RBSurf::AmbientFlush, surf->material, surf->materialRegisters, surf->entity, nullptr);

                prevMaterial = surf->material;
            }

            if (isDifferentEntity) {
                prevEntity = surf->entity;

                backEnd.modelViewMatrix = surf->entity->modelViewMatrix;
                backEnd.modelViewProjMatrix = surf->entity->modelViewProjMatrix;
                    
                bool depthHack = surf->entity->def->parms.depthHack;
            
                if (prevDepthHack != depthHack) {
                    if (depthHack) {
                        glr.SetDepthRange(0.0f, 0.1f);
                    } else {
                        glr.SetDepthRange(0.0f, 1.0f);
                    }

                    prevDepthHack = depthHack;
                }
            }

            prevSortkey = surf->sortkey;
        }
        
        backEnd.rbsurf.DrawSubMesh(surf->subMesh, surf->guiSubMesh);
    }

    if (prevMaterial) {
        backEnd.rbsurf.Flush();
    }

    // restore depthHack
    if (prevDepthHack) {
        glr.SetDepthRange(0.0f, 1.0f);
    }
}

void RB_BlendPass(int numDrawSurfs, DrawSurf **drawSurfs) {
    int                 prevSortkey = -1;
    viewEntity_t *      prevEntity = nullptr;
    const Material *    prevMaterial = nullptr;
    bool                prevDepthHack = false;

    for (int i = 0; i < numDrawSurfs; i++) {
        const DrawSurf *surf = drawSurfs[i];
        if (!(surf->flags & DrawSurf::AmbientVisible)) {
            continue;
        }

        if (surf->sortkey != prevSortkey) {
            if (!(surf->material->GetCoverage() & Material::TranslucentCoverage)) {
                continue;
            }

            bool isDifferentEntity = surf->entity != prevEntity ? true : false;
            bool isDifferentMaterial = surf->material != prevMaterial ? true : false;

            if (isDifferentMaterial || isDifferentEntity) {
                if (prevMaterial) {
                    backEnd.rbsurf.Flush();
                }

                backEnd.rbsurf.Begin(RBSurf::BlendFlush, surf->material, surf->materialRegisters, surf->entity, nullptr);

                prevMaterial = surf->material;
            }

            if (isDifferentEntity) {
                prevEntity = surf->entity;

                backEnd.modelViewMatrix = surf->entity->modelViewMatrix;
                backEnd.modelViewProjMatrix = surf->entity->modelViewProjMatrix;

                bool depthHack = surf->entity->def->parms.depthHack;
            
                if (prevDepthHack != depthHack) {
                    if (depthHack) {
                        glr.SetDepthRange(0.0f, 0.1f);
                    } else {
                        glr.SetDepthRange(0.0f, 1.0f);
                    }

                    prevDepthHack = depthHack;
                }
            }

            prevSortkey = surf->sortkey;
        }

        backEnd.rbsurf.DrawSubMesh(surf->subMesh, surf->guiSubMesh);
    }

    if (prevMaterial) {
        backEnd.rbsurf.Flush();
    }

    // restore depthHack
    if (prevDepthHack) {
        glr.SetDepthRange(0.0f, 1.0f);
    }
}

void RB_VelocityMapPass(int numDrawSurfs, DrawSurf **drawSurfs) {
    int                 prevSortkey = -1;
    viewEntity_t *      prevEntity = nullptr;
    viewEntity_t *      skipEntity = nullptr;
    const Material *    prevMaterial = nullptr;
    bool                firstDraw = true;

    for (int i = 0; i < numDrawSurfs; i++) {
        const DrawSurf *surf = drawSurfs[i];
        if (!(surf->flags & DrawSurf::AmbientVisible)) {
            continue;
        }

        if (surf->sortkey != prevSortkey) {
            if (surf->entity == skipEntity) {
                continue;
            }

            if (surf->material->GetCoverage() == Material::EmptyCoverage || surf->material->GetCoverage() == Material::TranslucentCoverage) {
                continue;
            }

            bool isDifferentEntity = surf->entity != prevEntity ? true : false;
            bool isDifferentMaterial = surf->material != prevMaterial ? true : false;
            
            if (isDifferentMaterial || isDifferentEntity) {
                if (!prevMaterial) {
                    backEnd.rbsurf.Begin(RBSurf::VelocityFlush, surf->material, surf->materialRegisters, surf->entity, nullptr);
                } else {
                    if (isDifferentEntity || prevMaterial->GetCullType() != surf->material->GetCullType() || 
                        (prevMaterial->GetCoverage() & Material::PerforatedCoverage) || (surf->material->GetCoverage() & Material::PerforatedCoverage)) {
                        backEnd.rbsurf.Flush();
                        backEnd.rbsurf.Begin(RBSurf::VelocityFlush, surf->material, surf->materialRegisters, surf->entity, nullptr);
                    }
                }

                prevMaterial = surf->material;
            }

            if (isDifferentEntity) {
                prevEntity = surf->entity;

                Mesh *mesh = surf->entity->def->parms.mesh;

                if (!mesh->IsSkinnedMesh() && (surf->entity->def->motionBlurModelMatrix[0] == surf->entity->def->motionBlurModelMatrix[1])) {
                    skipEntity = surf->entity;
                    continue;
                }

                skipEntity = nullptr;

                backEnd.modelViewMatrix = surf->entity->modelViewMatrix;
                backEnd.modelViewProjMatrix = surf->entity->modelViewProjMatrix;
            }

            prevSortkey = surf->sortkey;
        }

        if (firstDraw) {
            firstDraw = false;

            backEnd.ctx->ppRTs[PP_RT_VEL]->Begin();

            glr.SetViewport(Rect(0, 0, backEnd.ctx->ppRTs[PP_RT_VEL]->GetWidth(), backEnd.ctx->ppRTs[PP_RT_VEL]->GetHeight()));
            glr.SetStateBits(Renderer::ColorWrite | Renderer::AlphaWrite | Renderer::DepthWrite);
            glr.Clear(Renderer::ColorBit | Renderer::DepthBit, Color4(0.0f, 0.0f, 0.0f, 1.0f), 1.0f, 0);
        }

        backEnd.rbsurf.DrawSubMesh(surf->subMesh, surf->guiSubMesh);
    }

    if (!firstDraw) {
        backEnd.rbsurf.Flush();

        backEnd.ctx->ppRTs[PP_RT_VEL]->End();
        
        glr.SetViewport(backEnd.renderRect);
        glr.SetScissor(backEnd.renderRect);
    } else {
        firstDraw = false;

        backEnd.ctx->ppRTs[PP_RT_VEL]->Begin();

        glr.SetViewport(Rect(0, 0, backEnd.ctx->ppRTs[PP_RT_VEL]->GetWidth(), backEnd.ctx->ppRTs[PP_RT_VEL]->GetHeight()));
        glr.SetStateBits(Renderer::ColorWrite | Renderer::AlphaWrite | Renderer::DepthWrite);
        glr.Clear(Renderer::ColorBit | Renderer::DepthBit, Color4(0.0f, 0.0f, 0.0f, 1.0f), 1.0f, 0);
    
        backEnd.ctx->ppRTs[PP_RT_VEL]->End();

        glr.SetViewport(backEnd.renderRect);
        glr.SetScissor(backEnd.renderRect);
    }
}

void RB_FinalPass(int numDrawSurfs, DrawSurf **drawSurfs) {
    int                 prevSortkey = -1;
    viewEntity_t *      prevEntity = nullptr;
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
        
        if (surf->sortkey != prevSortkey) {
            if (surf->material->GetCoverage() == Material::EmptyCoverage) {
                continue;
            }

            if (1) {//!surf->material->HasRefraction()) {
                continue;
            }

            bool isDifferentEntity = surf->entity != prevEntity ? true : false;
            bool isDifferentMaterial = surf->material != prevMaterial ? true : false;

            if (isDifferentMaterial || isDifferentEntity) {
                if (prevMaterial) {
                    backEnd.rbsurf.Flush();
                }

                backEnd.rbsurf.Begin(RBSurf::FinalFlush, surf->material, surf->materialRegisters, surf->entity, nullptr);

                prevMaterial = surf->material;
            }

            if (isDifferentEntity) {
                prevEntity = surf->entity;

                backEnd.modelViewMatrix = surf->entity->modelViewMatrix;
                backEnd.modelViewProjMatrix = surf->entity->modelViewProjMatrix;

                bool depthHack = surf->entity->def->parms.depthHack;
                
                if (prevDepthHack != depthHack) {
                    if (depthHack) {
                        glr.SetDepthRange(0.0f, 0.1f);
                    } else {
                        glr.SetDepthRange(0.0f, 1.0f);
                    }

                    prevDepthHack = depthHack;
                }
            }

            prevSortkey = surf->sortkey;
        }

        backEnd.rbsurf.DrawSubMesh(surf->subMesh, surf->guiSubMesh);
    }

    if (prevMaterial) {
        backEnd.rbsurf.Flush();
    }

    // restore depthHack
    if (prevDepthHack) {
        glr.SetDepthRange(0.0f, 1.0f);
    }
}

void RB_GuiPass(int numDrawSurfs, DrawSurf **drawSurfs) {
    int                 prevSortkey = -1;
    viewEntity_t *      prevEntity = nullptr;
    const Material *    prevMaterial = nullptr;
    bool                prevDepthHack = false;
        
    for (int i = 0; i < numDrawSurfs; i++) {
        const DrawSurf *surf = drawSurfs[i];
        if (surf->sortkey != prevSortkey) {
            bool isDifferentEntity = surf->entity != prevEntity ? true : false;
            bool isDifferentMaterial = surf->material != prevMaterial ? true : false;

            if (isDifferentMaterial || isDifferentEntity) {
                if (prevMaterial) {
                    backEnd.rbsurf.Flush();
                }

                backEnd.rbsurf.Begin(RBSurf::GuiFlush, surf->material, surf->materialRegisters, surf->entity, nullptr);

                prevMaterial = surf->material;
            }

            if (isDifferentEntity) {
                prevEntity = surf->entity;

                backEnd.modelViewMatrix = surf->entity->modelViewMatrix;
                backEnd.modelViewProjMatrix = surf->entity->modelViewProjMatrix;

                bool depthHack = surf->entity->def->parms.depthHack;
                
                if (prevDepthHack != depthHack) {
                    if (depthHack) {
                        glr.SetDepthRange(0.0f, 0.1f);
                    } else {
                        glr.SetDepthRange(0.0f, 1.0f);
                    }

                    prevDepthHack = depthHack;
                }
            }

            prevSortkey = surf->sortkey;
        }
        
        backEnd.rbsurf.DrawSubMesh(surf->subMesh, surf->guiSubMesh);
    }

    if (prevMaterial) {
        backEnd.rbsurf.Flush();
    }

    // restore depthHack
    if (prevDepthHack) {
        glr.SetDepthRange(0.0f, 1.0f);
    }
}

BE_NAMESPACE_END
