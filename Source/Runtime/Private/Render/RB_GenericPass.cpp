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
#include "Profiler/Profiler.h"

BE_NAMESPACE_BEGIN

// Render sky only
void RB_BackgroundPass(int numDrawSurfs, DrawSurf **drawSurfs) {
    const VisObject *   prevSpace = nullptr;
    const Material *    prevMaterial = nullptr;

    backEnd.batch.SetCurrentLight(nullptr);

    for (int i = 0; i < numDrawSurfs; i++) {
        const DrawSurf *drawSurf = drawSurfs[i];

        if (drawSurf->material->GetSort() != Material::Sort::Sky) {
            continue;
        }

        bool isDifferentObject = drawSurf->space != prevSpace;
        bool isDifferentMaterial = drawSurf->material != prevMaterial;

        if (isDifferentMaterial || isDifferentObject) {
            if (prevMaterial) {
                backEnd.batch.Flush();
            }

            backEnd.batch.Begin(Batch::Flush::Background, drawSurf->material, drawSurf->materialRegisters, drawSurf->space);

            prevMaterial = drawSurf->material;

            if (isDifferentObject) {
                prevSpace = drawSurf->space;

                backEnd.modelViewMatrix = drawSurf->space->modelViewMatrix;
                backEnd.modelViewProjMatrix = drawSurf->space->modelViewProjMatrix;
            }
        }

        backEnd.batch.DrawSubMesh(drawSurf->subMesh);
    }

    // Flush previous batch
    if (prevMaterial) {
        backEnd.batch.Flush();
    }
}

void RB_SelectionPass(int numDrawSurfs, DrawSurf **drawSurfs) {
    const VisObject *   prevSpace = nullptr;
    const Material *    prevMaterial = nullptr;
    bool                prevDepthHack = false;

    backEnd.batch.SetCurrentLight(nullptr);

    for (int i = 0; i < numDrawSurfs; i++) {
        const DrawSurf *drawSurf = drawSurfs[i];

        if ((drawSurf->flags & DrawSurf::Flag::SkipSelection) || !(drawSurf->flags & DrawSurf::Flag::Visible)) {
            continue;
        }

        if (drawSurf->material->GetSort() == Material::Sort::Sky) {
            continue;
        }

        bool isDifferentObject = drawSurf->space != prevSpace;
        bool isDifferentMaterial = drawSurf->material != prevMaterial;

        if (isDifferentMaterial || isDifferentObject) {
            if (drawSurf->space->def->GetState().flags & RenderObject::Flag::SkipSelection) {
                continue;
            }

            if (!prevMaterial) {
                backEnd.batch.Begin(Batch::Flush::Selection, drawSurf->material, drawSurf->materialRegisters, drawSurf->space);
            } else {
                if (isDifferentObject || prevMaterial->GetCullType() != drawSurf->material->GetCullType() ||
                    (prevMaterial->GetSort() == Material::Sort::AlphaTest) || (drawSurf->material->GetSort() == Material::Sort::AlphaTest)) {
                    backEnd.batch.Flush();
                    backEnd.batch.Begin(Batch::Flush::Selection, drawSurf->material, drawSurf->materialRegisters, drawSurf->space);
                }
            }

            prevMaterial = drawSurf->material;

            if (isDifferentObject) {
                bool depthHack = !!(drawSurf->space->def->GetState().flags & RenderObject::Flag::DepthHack);

                if (prevDepthHack != depthHack) {
                    if (depthHack) {
                        rhi.SetDepthRange(0.0f, 0.1f);
                    } else {
                        rhi.SetDepthRange(0.0f, 1.0f);
                    }

                    prevDepthHack = depthHack;
                }

                backEnd.modelViewMatrix = drawSurf->space->modelViewMatrix;
                backEnd.modelViewProjMatrix = drawSurf->space->modelViewProjMatrix;

                prevSpace = drawSurf->space;
            }
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

void RB_OccluderPass(int numDrawSurfs, DrawSurf **drawSurfs) {
    const VisObject *   prevSpace = nullptr;
    const Material *    prevMaterial = nullptr;
    bool                prevDepthHack = false;

    backEnd.batch.SetCurrentLight(nullptr);

    for (int i = 0; i < numDrawSurfs; i++) {
        const DrawSurf *drawSurf = drawSurfs[i];

        if (drawSurf->material->GetSort() != Material::Sort::Opaque) {
            continue;
        }

        bool isDifferentObject = drawSurf->space != prevSpace;
        bool isDifferentMaterial = drawSurf->material != prevMaterial;
            
        if (isDifferentMaterial || isDifferentObject) {
            if (!(drawSurf->space->def->GetState().flags & RenderObject::Flag::Occluder)) {
                continue;
            }

            /*if (surf->subMesh->GetAABB().Volume() < MeterToUnit(1.0f) * MeterToUnit(1.0f) * MeterToUnit(1.0f)) {
                continue;
            }*/

            if (!prevMaterial) {
                backEnd.batch.Begin(Batch::Flush::Occluder, drawSurf->material, drawSurf->materialRegisters, drawSurf->space);
            } else {
                if (isDifferentObject) {
                    backEnd.batch.Flush();
                    backEnd.batch.Begin(Batch::Flush::Occluder, drawSurf->material, drawSurf->materialRegisters, drawSurf->space);
                }
            }

            prevMaterial = drawSurf->material;

            if (isDifferentObject) {
                bool depthHack = !!(drawSurf->space->def->GetState().flags & RenderObject::Flag::DepthHack);

                if (prevDepthHack != depthHack) {
                    if (depthHack) {
                        rhi.SetDepthRange(0.0f, 0.1f);
                    } else {
                        rhi.SetDepthRange(0.0f, 1.0f);
                    }

                    prevDepthHack = depthHack;
                }

                backEnd.modelViewMatrix = drawSurf->space->modelViewMatrix;
                backEnd.modelViewProjMatrix = drawSurf->space->modelViewProjMatrix;

                prevSpace = drawSurf->space;
            }
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

void RB_DepthPrePass(int numDrawSurfs, DrawSurf **drawSurfs) {
    BE_PROFILE_CPU_SCOPE_STATIC("RB_DepthPrePass");
    BE_PROFILE_GPU_SCOPE_STATIC("RB_DepthPrePass");

    if (r_usePostProcessing.GetBool() && r_SSAO.GetBool()) {
        backEnd.ctx->screenRT->SetMRTMask(3);
    }

    const VisObject *   prevSpace = nullptr;
    const SubMesh *     prevSubMesh = nullptr;
    const Material *    prevMaterial = nullptr;
    bool                prevDepthHack = false;
    Batch::Flush::Enum  drawType;

    backEnd.batch.SetCurrentLight(nullptr);

    for (int i = 0; i < numDrawSurfs; i++) {
        const DrawSurf *drawSurf = drawSurfs[i];

        if (drawSurf->material->IsSkySurface()) {
            continue;
        }

        if (drawSurf->material->IsLitSurface()) {
            if (!backEnd.useDepthPrePass) {
                continue;
            }

            drawType = Batch::Flush::Depth;
        } else if (drawSurf->material->GetRenderingMode() != Material::RenderingMode::AlphaBlend) {
            drawType = Batch::Flush::Unlit;
        } else {
            continue;
        }

        bool isDifferentObject = drawSurf->space != prevSpace;
        bool isDifferentSubMesh = prevSubMesh ? !drawSurf->subMesh->IsShared(prevSubMesh) : true;
        bool isDifferentMaterial = drawSurf->material != prevMaterial;
        bool isDifferentInstance = !(drawSurf->flags & DrawSurf::Flag::UseInstancing) || isDifferentMaterial || isDifferentSubMesh || !prevSpace ||
            prevSpace->def->GetState().flags != drawSurf->space->def->GetState().flags || prevSpace->def->GetState().layer != drawSurf->space->def->GetState().layer ? true : false;

        if (isDifferentObject || isDifferentSubMesh || isDifferentMaterial) {
            if (prevMaterial && isDifferentInstance) {
                backEnd.batch.Flush();
            }

            backEnd.batch.Begin(drawType, drawSurf->material, drawSurf->materialRegisters, drawSurf->space);

            prevSubMesh = drawSurf->subMesh;
            prevMaterial = drawSurf->material;

            if (isDifferentObject) {
                bool depthHack = !!(drawSurf->space->def->GetState().flags & RenderObject::Flag::DepthHack);

                if (prevDepthHack != depthHack) {
                    if (drawSurf->flags & DrawSurf::Flag::UseInstancing) {
                        backEnd.batch.Flush();
                    }

                    if (depthHack) {
                        rhi.SetDepthRange(0.0f, 0.1f);
                    } else {
                        rhi.SetDepthRange(0.0f, 1.0f);
                    }

                    prevDepthHack = depthHack;
                }

                if (!(drawSurf->flags & DrawSurf::Flag::UseInstancing)) {
                    backEnd.modelViewMatrix = drawSurf->space->modelViewMatrix;
                    backEnd.modelViewProjMatrix = drawSurf->space->modelViewProjMatrix;
                }

                prevSpace = drawSurf->space;
            }
        }

        if (drawSurf->flags & DrawSurf::Flag::UseInstancing) {
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

    if (r_usePostProcessing.GetBool() && r_SSAO.GetBool()) {
        backEnd.ctx->screenRT->SetMRTMask(1);
    }
}

void RB_BlendPass(int numDrawSurfs, DrawSurf **drawSurfs) {
    if (r_skipBlendPass.GetBool()) {
        return;
    }

    BE_PROFILE_CPU_SCOPE_STATIC("RB_BlendPass");
    BE_PROFILE_GPU_SCOPE_STATIC("RB_BlendPass");

    const VisObject *   prevSpace = nullptr;
    const SubMesh *     prevSubMesh = nullptr;
    const Material *    prevMaterial = nullptr;
    bool                prevDepthHack = false;
    Batch::Flush::Enum  flushType = Batch::Flush::Unlit;

    backEnd.batch.SetCurrentLight(nullptr);

    for (int i = 0; i < numDrawSurfs; i++) {
        const DrawSurf *drawSurf = drawSurfs[i];

        if (drawSurf->material->IsLitSurface()) {
            continue;
        }

        if (drawSurf->material->GetRenderingMode() != Material::RenderingMode::AlphaBlend) {
            continue;
        }

        bool isDifferentObject = drawSurf->space != prevSpace;
        bool isDifferentSubMesh = prevSubMesh ? !drawSurf->subMesh->IsShared(prevSubMesh) : true;
        bool isDifferentMaterial = drawSurf->material != prevMaterial;
        bool isDifferentInstance = !(drawSurf->flags & DrawSurf::Flag::UseInstancing) || isDifferentMaterial || isDifferentSubMesh || !prevSpace ||
            prevSpace->def->GetState().flags != drawSurf->space->def->GetState().flags || prevSpace->def->GetState().layer != drawSurf->space->def->GetState().layer ? true : false;

        if (isDifferentObject || isDifferentSubMesh || isDifferentMaterial) {
            if (prevMaterial && isDifferentInstance) {
                backEnd.batch.Flush();
            }

            backEnd.batch.Begin(flushType, drawSurf->material, drawSurf->materialRegisters, drawSurf->space);

            prevSubMesh = drawSurf->subMesh;
            prevMaterial = drawSurf->material;

            if (isDifferentObject) {
                bool depthHack = !!(drawSurf->space->def->GetState().flags & RenderObject::Flag::DepthHack);

                if (prevDepthHack != depthHack) {
                    if (drawSurf->flags & DrawSurf::Flag::UseInstancing) {
                        backEnd.batch.Flush();
                    }

                    if (depthHack) {
                        rhi.SetDepthRange(0.0f, 0.1f);
                    } else {
                        rhi.SetDepthRange(0.0f, 1.0f);
                    }

                    prevDepthHack = depthHack;
                }

                if (!(drawSurf->flags & DrawSurf::Flag::UseInstancing)) {
                    backEnd.modelViewMatrix = drawSurf->space->modelViewMatrix;
                    backEnd.modelViewProjMatrix = drawSurf->space->modelViewProjMatrix;
                }

                prevSpace = drawSurf->space;
            }
        }

        if (drawSurf->flags & DrawSurf::Flag::UseInstancing) {
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

void RB_VelocityMapPass(int numDrawSurfs, DrawSurf **drawSurfs) {
    if ((backEnd.camera->def->GetState().flags & RenderCamera::Flag::SkipPostProcess) ||
        !r_usePostProcessing.GetBool() || !(r_motionBlur.GetInteger() & 2)) {
        return;
    }

    if (backEnd.camera->def->GetState().clearMethod != RenderCamera::ClearMethod::Color ||
        backEnd.camera->def->GetState().clearMethod != RenderCamera::ClearMethod::Skybox) {
        return;
    }

    BE_PROFILE_CPU_SCOPE_STATIC("RB_VelocityMapPass");
    BE_PROFILE_GPU_SCOPE_STATIC("RB_VelocityMapPass");

    const VisObject *   prevSpace = nullptr;
    const VisObject *   skipObject = nullptr;
    const Material *    prevMaterial = nullptr;
    bool                firstDraw = true;

    backEnd.batch.SetCurrentLight(nullptr);

    for (int i = 0; i < numDrawSurfs; i++) {
        const DrawSurf *drawSurf = drawSurfs[i];

        if (drawSurf->space == skipObject) {
            continue;
        }

        if (drawSurf->material->GetSort() == Material::Sort::Sky) {
            continue;
        }

        bool isDifferentObject = drawSurf->space != prevSpace;
        bool isDifferentMaterial = drawSurf->material != prevMaterial;
            
        if (isDifferentMaterial || isDifferentObject) {
            if (!prevMaterial) {
                backEnd.batch.Begin(Batch::Flush::Velocity, drawSurf->material, drawSurf->materialRegisters, drawSurf->space);
            } else {
                if (isDifferentObject || prevMaterial->GetCullType() != drawSurf->material->GetCullType() ||
                    (prevMaterial->GetSort() == Material::Sort::AlphaTest) || (drawSurf->material->GetSort() == Material::Sort::AlphaTest)) {
                    backEnd.batch.Flush();
                    backEnd.batch.Begin(Batch::Flush::Velocity, drawSurf->material, drawSurf->materialRegisters, drawSurf->space);
                }
            }

            prevMaterial = drawSurf->material;

            if (isDifferentObject) {
                Mesh *mesh = drawSurf->space->def->GetState().mesh;

                if (!mesh->IsSkinnedMesh() && (drawSurf->space->def->GetWorldMatrix() == drawSurf->space->def->GetPrevWorldMatrix())) {
                    skipObject = drawSurf->space;
                    continue;
                }

                skipObject = nullptr;

                backEnd.modelViewMatrix = drawSurf->space->modelViewMatrix;
                backEnd.modelViewProjMatrix = drawSurf->space->modelViewProjMatrix;

                prevSpace = drawSurf->space;
            }
        }

        if (firstDraw) {
            firstDraw = false;

            backEnd.ctx->ppRTs[PP_RT_VEL]->Begin();

            rhi.SetViewport(Rect(0, 0, backEnd.ctx->ppRTs[PP_RT_VEL]->GetWidth(), backEnd.ctx->ppRTs[PP_RT_VEL]->GetHeight()));
            rhi.SetStateBits(RHI::ColorWrite | RHI::AlphaWrite | RHI::DepthWrite);
            rhi.Clear(RHI::ClearBit::Color | RHI::ClearBit::Depth, Color4(0.0f, 0.0f, 0.0f, 1.0f), 1.0f, 0);
        }

        backEnd.batch.DrawSubMesh(drawSurf->subMesh);
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
        rhi.Clear(RHI::ClearBit::Color | RHI::ClearBit::Depth, Color4(0.0f, 0.0f, 0.0f, 1.0f), 1.0f, 0);
    
        backEnd.ctx->ppRTs[PP_RT_VEL]->End();

        rhi.SetViewport(backEnd.renderRect);
        rhi.SetScissor(backEnd.renderRect);
    }
}

void RB_FinalPass(int numDrawSurfs, DrawSurf **drawSurfs) {
    if (r_skipFinalPass.GetBool()) {
        return;
    }

    BE_PROFILE_CPU_SCOPE_STATIC("RB_FinalPass");
    BE_PROFILE_GPU_SCOPE_STATIC("RB_FinalPass");

    const VisObject *   prevSpace = nullptr;
    const SubMesh *     prevSubMesh = nullptr;
    const Material *    prevMaterial = nullptr;
    bool                prevDepthHack = false;

    backEnd.batch.SetCurrentLight(nullptr);
    
    if (backEnd.ctx->updateCurrentRenderTexture) {
        backEnd.ctx->updateCurrentRenderTexture = false;
        backEnd.ctx->UpdateCurrentRenderTexture();
    }
        
    for (int i = 0; i < numDrawSurfs; i++) {
        const DrawSurf *drawSurf = drawSurfs[i];
        
        if (drawSurf->material->GetSort() == Material::Sort::Sky) {
            continue;
        }

        if (1) {//!surf->material->HasRefraction()) {
            continue;
        }

        bool isDifferentObject = drawSurf->space != prevSpace;
        bool isDifferentSubMesh = prevSubMesh ? !drawSurf->subMesh->IsShared(prevSubMesh) : true;
        bool isDifferentMaterial = drawSurf->material != prevMaterial;
        bool isDifferentInstance = !(drawSurf->flags & DrawSurf::Flag::UseInstancing) || isDifferentMaterial || isDifferentSubMesh || !prevSpace ||
            prevSpace->def->GetState().flags != drawSurf->space->def->GetState().flags || prevSpace->def->GetState().layer != drawSurf->space->def->GetState().layer ? true : false;

        if (isDifferentObject || isDifferentSubMesh || isDifferentMaterial) {
            if (prevMaterial && isDifferentInstance) {
                backEnd.batch.Flush();
            }

            backEnd.batch.Begin(Batch::Flush::Final, drawSurf->material, drawSurf->materialRegisters, drawSurf->space);

            prevSubMesh = drawSurf->subMesh;
            prevMaterial = drawSurf->material;

            if (isDifferentObject) {
                bool depthHack = !!(drawSurf->space->def->GetState().flags & RenderObject::Flag::DepthHack);

                if (prevDepthHack != depthHack) {
                    if (drawSurf->flags & DrawSurf::Flag::UseInstancing) {
                        backEnd.batch.Flush();
                    }

                    if (depthHack) {
                        rhi.SetDepthRange(0.0f, 0.1f);
                    } else {
                        rhi.SetDepthRange(0.0f, 1.0f);
                    }

                    prevDepthHack = depthHack;
                }

                if (!(drawSurf->flags & DrawSurf::Flag::UseInstancing)) {
                    backEnd.modelViewMatrix = drawSurf->space->modelViewMatrix;
                    backEnd.modelViewProjMatrix = drawSurf->space->modelViewProjMatrix;
                }

                prevSpace = drawSurf->space;
            }
        }

        if (drawSurf->flags & DrawSurf::Flag::UseInstancing) {
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

void RB_GuiPass(int numDrawSurfs, DrawSurf **drawSurfs) {
    const VisObject *   prevSpace = nullptr;
    const Material *    prevMaterial = nullptr;
    bool                prevDepthHack = false;

    backEnd.batch.SetCurrentLight(nullptr);
        
    for (int i = 0; i < numDrawSurfs; i++) {
        const DrawSurf *drawSurf = drawSurfs[i];

        bool isDifferentObject = drawSurf->space != prevSpace;
        bool isDifferentMaterial = drawSurf->material != prevMaterial;

        if (isDifferentMaterial || isDifferentObject) {
            if (prevMaterial) {
                backEnd.batch.Flush();
            }

            backEnd.batch.Begin(Batch::Flush::Gui, drawSurf->material, drawSurf->materialRegisters, drawSurf->space);

            prevMaterial = drawSurf->material;

            if (isDifferentObject) {
                bool depthHack = !!(drawSurf->space->def->GetState().flags & RenderObject::Flag::DepthHack);

                if (prevDepthHack != depthHack) {
                    if (depthHack) {
                        rhi.SetDepthRange(0.0f, 0.1f);
                    } else {
                        rhi.SetDepthRange(0.0f, 1.0f);
                    }

                    prevDepthHack = depthHack;
                }

                backEnd.modelViewMatrix = drawSurf->space->modelViewMatrix;
                backEnd.modelViewProjMatrix = drawSurf->space->modelViewProjMatrix;

                prevSpace = drawSurf->space;
            }
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

BE_NAMESPACE_END
