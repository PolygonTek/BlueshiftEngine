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

// sceneEntity 로 부터 viewEntity 를 등록, 같은 view 에 여러번 등록 방지
viewEntity_t *RenderWorld::RegisterViewEntity(view_t *view, SceneEntity *sceneEntity) {
    if (sceneEntity->viewCount == viewCount) {
        // already registered for this frame
        return sceneEntity->viewEntity;
    }

    viewEntity_t *viewEntity = (viewEntity_t *)frameData.ClearedAlloc(sizeof(*viewEntity));
    viewEntity->def = sceneEntity;

    // view 에 linked list 로 연결
    viewEntity->next = view->viewEntities;
    view->viewEntities = viewEntity;

    // viewEntity 를 sceneEntity 에 연결
    sceneEntity->viewEntity = viewEntity;

    // RenderView 에서 1 씩 증가되는 viewCount
    sceneEntity->viewCount = viewCount;

    return viewEntity;
}

// sceneLight 로 부터 viewLight 를 등록, 같은 view 에 여러번 등록 방지
viewLight_t *RenderWorld::RegisterViewLight(view_t *view, SceneLight *sceneLight) {
    if (sceneLight->viewCount == viewCount) {
        // already registered for this frame
        return sceneLight->viewLight;
    }
    
    viewLight_t *viewLight = (viewLight_t *)frameData.ClearedAlloc(sizeof(*viewLight));
    viewLight->def = sceneLight;

    // view 에 linked list 로 연결
    viewLight->next = view->viewLights;
    view->viewLights = viewLight;

    // viewLight 를 sceneLight 에 연결
    sceneLight->viewLight = viewLight;

    // RenderView 에서 1 씩 증가되는 viewCount
    sceneLight->viewCount = viewCount;

    return viewLight;
}

// viewLight 와 viewEntity 들을 등록한다.
void RenderWorld::FindViewLightsAndEntities(view_t *view) {
    viewCount++;

    view->aabb.Clear();
    view->viewLights = nullptr;
    view->viewEntities = nullptr;

    // Called for each lights intersecting with view frustum 
    // Returns true if it want to proceed next query
    auto addViewLights = [this, view](int32_t proxyId) -> bool {
        DbvtProxy *proxy = (DbvtProxy *)lightDbvt.GetUserData(proxyId);
        SceneLight *sceneLight = proxy->sceneLight;

        if (!sceneLight) {
            return true;
        }

        if (!(BIT(sceneLight->parms.layer) & view->def->parms.layerMask)) {
            return true;
        }

        if (!sceneLight->parms.turnOn) {
            return true;
        }

        // Skip if a light is farther than maximum visible distance
        if (sceneLight->parms.origin.DistanceSqr(view->def->parms.origin) > sceneLight->parms.maxVisDist * sceneLight->parms.maxVisDist) {
            return true;
        }

        // Cull exact light bounding volume
        if (view->def->parms.orthogonal) {
            if (sceneLight->Cull(view->def->box)) {
                return true;
            }
        } else {
            if (sceneLight->Cull(view->def->frustum)) {
                return true;
            }
        }

        // light scissorRect 계산
        Rect screenClipRect;
        if (!sceneLight->ComputeScreenClipRect(view->def, screenClipRect)) {
            return true;
        }

        viewLight_t *viewLight = RegisterViewLight(view, sceneLight);
        viewLight->scissorRect = screenClipRect;
        // glScissor 의 x, y 좌표는 lower left corner 이므로 y 좌표를 밑에서 증가되도록 뒤집는다.
        viewLight->scissorRect.y = renderSystem.currentContext->GetRenderingHeight() - (screenClipRect.y + screenClipRect.h);

        viewLight->litSurfsAABB.Clear();
        viewLight->shadowCasterAABB.Clear();

        return true;
    };

    // Called for each entities intersecting with view frustum 
    // Returns true if it want to proceed next query
    auto addViewEntities = [this, view](int32_t proxyId) -> bool {
        DbvtProxy *proxy = (DbvtProxy *)entityDbvt.GetUserData(proxyId);
        SceneEntity *sceneEntity = proxy->sceneEntity;

        if (!sceneEntity) {
            return true;
        }

        if (!(BIT(sceneEntity->parms.layer) & view->def->parms.layerMask)) {
            return true;
        }

        // Skip first person view only entity in subView 
        if (sceneEntity->parms.firstPersonOnly && view->isSubview) {
            return true;
        }

        // Skip 3rd person view only entity in subView
        if (sceneEntity->parms.thirdPersonOnly && !view->isSubview) {
            return true;
        }

        // Skip if a entity is farther than maximum visible distance
        if (sceneEntity->parms.origin.DistanceSqr(view->def->parms.origin) > sceneEntity->parms.maxVisDist * sceneEntity->parms.maxVisDist) {
            return true;
        }

        viewEntity_t *viewEntity = RegisterViewEntity(view, sceneEntity);
            
        viewEntity->ambientVisible = true;

        viewEntity->modelViewMatrix = view->def->viewMatrix * sceneEntity->GetModelMatrix();
        viewEntity->modelViewProjMatrix = view->def->viewProjMatrix * sceneEntity->GetModelMatrix();

        if (sceneEntity->parms.billboard) {
            Mat3 inverse = (view->def->viewMatrix.ToMat3() * sceneEntity->GetModelMatrix().ToMat3()).Inverse();
            //inverse = inverse * Mat3(0, 0, 1, 1, 0, 0, 0, 1, 0);
            Swap(inverse[0], inverse[2]);
            Swap(inverse[1], inverse[2]);

            Mat3 billboardMatrix = inverse * Mat3::FromScale(sceneEntity->parms.scale);
            viewEntity->modelViewMatrix *= billboardMatrix;
            viewEntity->modelViewProjMatrix *= billboardMatrix;
        }

        view->aabb.AddAABB(proxy->aabb);

        if (r_showAABB.GetInteger() > 0) {
            SetDebugColor(Color4::blue, Color4::zero);
            DebugAABB(proxy->aabb, 1, true, r_showAABB.GetInteger() == 1 ? true : false);
        }

        if (viewEntity->def->parms.numJoints > 0 && r_showSkeleton.GetInteger() > 0) {
            DebugJoints(viewEntity->def, r_showSkeleton.GetInteger() == 2, view->def->parms.axis);
        }

        return true;
    };

    if (view->def->parms.orthogonal) {
        lightDbvt.Query(view->def->box, addViewLights);
        entityDbvt.Query(view->def->box, addViewEntities);
    } else {
        lightDbvt.Query(view->def->frustum, addViewLights);
        entityDbvt.Query(view->def->frustum, addViewEntities);
    }
}

// static mesh 들을 ambient drawSurfs 에 담는다.
void RenderWorld::AddStaticMeshes(view_t *view) {
    // Called for each static mesh surfaces intersecting with view frustum 
    // Returns true if it want to proceed next query
    auto addStaticMeshSurfs = [this, view](int32_t proxyId) -> bool {
        DbvtProxy *proxy = (DbvtProxy *)staticMeshDbvt.GetUserData(proxyId);
        MeshSurf *surf = proxy->mesh->GetSurface(proxy->meshSurfIndex);

        // surf 가 없다면 static mesh 가 아님
        if (!surf) {
            return true;
        }

        if (proxy->sceneEntity->viewCount != this->viewCount) {
            return true;
        }

        /*if (proxy->lodGroup >= 0) {
            // Compute LOD value [0, 1]
            AABB aabb = surf->subMesh->GetAABB() * proxy->sceneEntity->parms.scale;
            Vec3 size = aabb[1] - aabb[0];
            float area = view->def->parms.axis[0].Abs().Dot(Vec3(size.y * size.z, size.z * size.x, size.x * size.y));
            float d = view->def->parms.origin.DistanceSqr(proxy->sceneEntity->parms.origin);
            float lodValue = area / d;
        }*/

#if 0
        // More accurate OBB culling
        OBB obb = OBB(proxy->sceneEntity->GetAABB(), proxy->sceneEntity->parms.origin, proxy->sceneEntity->parms.axis);
        if (view->def->frustum.CullOBB(obb)) {
            return true;
        }
#endif

        int flags = DrawSurf::AmbientVisible;
        if (proxy->sceneEntity->parms.wireframeMode != SceneEntity::WireframeMode::ShowNone || r_showWireframe.GetInteger() > 0) {
            flags |= DrawSurf::ShowWires;
        }

        viewEntity_t *viewEntity = proxy->sceneEntity->viewEntity;
        AddDrawSurf(view, viewEntity, viewEntity->def->parms.materials[surf->materialIndex], surf->subMesh, flags);

        surf->viewCount = this->viewCount;
        surf->drawSurf = view->drawSurfs[view->numDrawSurfs - 1];
        //bool has = surf->drawSurf->material->IsLitSurface();

        if (r_showAABB.GetInteger() > 0) {
            SetDebugColor(Color4(1, 1, 1, 0.5), Color4::zero);
            DebugAABB(proxy->aabb, 1, true, r_showAABB.GetInteger() == 1 ? true : false);
        }

        return true;
    };

    if (view->def->parms.orthogonal) {
        staticMeshDbvt.Query(view->def->box, addStaticMeshSurfs);
    } else {
        staticMeshDbvt.Query(view->def->frustum, addStaticMeshSurfs);
    }
}

// skinned mesh 들을 ambient drawSurfs 에 담는다. 
void RenderWorld::AddSkinnedMeshes(view_t *view) {
    for (viewEntity_t *viewEntity = view->viewEntities; viewEntity; viewEntity = viewEntity->next) {
        if (!viewEntity->ambientVisible) {
            continue;
        }

        const SceneEntity::Parms &entityParms = viewEntity->def->parms;

        if (!entityParms.mesh || !entityParms.joints) {
            continue;
        }

#if 0
        // More accurate OBB culling
        OBB obb = OBB(viewEntity->def->GetAABB(), entityParms.origin, entityParms.axis);
        if (view->def->frustum.CullOBB(obb)) {
            continue;
        }
#endif

        int flags = DrawSurf::AmbientVisible;
        if (entityParms.wireframeMode != SceneEntity::WireframeMode::ShowNone || r_showWireframe.GetInteger() > 0) {
            flags |= DrawSurf::ShowWires;
        }

        if (entityParms.skeleton && entityParms.joints) {
            // Update skinning joint cache for GPU skinning
            entityParms.mesh->UpdateSkinningJointCache(entityParms.skeleton, entityParms.joints);
        }

        for (int surfaceIndex = 0; surfaceIndex < entityParms.mesh->NumSurfaces(); surfaceIndex++) {
            MeshSurf *surf = entityParms.mesh->GetSurface(surfaceIndex);

            AddDrawSurf(view, viewEntity, entityParms.materials[surf->materialIndex], surf->subMesh, flags);

            surf->viewCount = viewCount;
            surf->drawSurf = view->drawSurfs[view->numDrawSurfs - 1];
        }
    }
}

void RenderWorld::AddParticleMeshes(view_t *view) {
    for (viewEntity_t *viewEntity = view->viewEntities; viewEntity; viewEntity = viewEntity->next) {
        if (!viewEntity->ambientVisible) {
            continue;
        }

        const SceneEntity::Parms &entityParms = viewEntity->def->parms;

        if (!entityParms.particleSystem) {
            continue;
        }

        int flags = DrawSurf::AmbientVisible | DrawSurf::SkipSelection;
        if (entityParms.wireframeMode != SceneEntity::WireframeMode::ShowNone || r_showWireframe.GetInteger() > 0) {
            flags |= DrawSurf::ShowWires;
        }

        particleMesh.Clear();
        particleMesh.Draw(entityParms.particleSystem, entityParms.stageParticles, viewEntity->def, view->def);
        particleMesh.CacheIndexes();

        for (int surfaceIndex = 0; surfaceIndex < particleMesh.surfaces.Count(); surfaceIndex++) {
            PrtMeshSurf *prtMeshSurf = &particleMesh.surfaces[surfaceIndex];
            if (!prtMeshSurf->numIndexes) {
                break;
            }

            // Copy this SubMesh to the temporary frame data for use in backend
            SubMesh *subMesh        = (SubMesh *)frameData.ClearedAlloc(sizeof(SubMesh));
            subMesh->alloced        = false;
            subMesh->type           = Mesh::DynamicMesh;
            subMesh->numIndexes     = prtMeshSurf->numIndexes;
            subMesh->numVerts       = prtMeshSurf->numVerts;

            subMesh->vertexCache    = (BufferCache *)frameData.ClearedAlloc(sizeof(BufferCache));
            *(subMesh->vertexCache) = prtMeshSurf->vertexCache;

            subMesh->indexCache     = (BufferCache *)frameData.ClearedAlloc(sizeof(BufferCache));
            *(subMesh->indexCache)  = prtMeshSurf->indexCache;

            AddDrawSurf(view, viewEntity, prtMeshSurf->material, subMesh, flags);
        }
    }
}

void RenderWorld::AddTextMeshes(view_t *view) {
    for (viewEntity_t *viewEntity = view->viewEntities; viewEntity; viewEntity = viewEntity->next) {
        if (!viewEntity->ambientVisible) {
            continue;
        }

        const SceneEntity::Parms &entityParms = viewEntity->def->parms;

        if (entityParms.text.IsEmpty()) {
            continue;
        }

        int flags = DrawSurf::AmbientVisible;
        if (entityParms.wireframeMode != SceneEntity::WireframeMode::ShowNone || r_showWireframe.GetInteger() > 0) {
            flags |= DrawSurf::ShowWires;
        }

        textMesh.Clear();
        textMesh.SetColor(Color4(&entityParms.materialParms[SceneEntity::RedParm]));
        textMesh.Draw(entityParms.font, entityParms.textAnchor, entityParms.textAlignment, entityParms.lineSpacing, entityParms.textScale, entityParms.text);
        textMesh.CacheIndexes();

        for (int surfaceIndex = 0; surfaceIndex < textMesh.NumSurfaces(); surfaceIndex++) {
            const GuiMeshSurf *guiMeshSurf = textMesh.Surface(surfaceIndex);
            if (!guiMeshSurf->numIndexes) {
                break;
            }

            // Copy this SubMesh to the temporary frame data for use in back end
            SubMesh *subMesh        = (SubMesh *)frameData.ClearedAlloc(sizeof(SubMesh));
            subMesh->alloced        = false;
            subMesh->type           = Mesh::DynamicMesh;
            subMesh->numIndexes     = guiMeshSurf->numIndexes;
            subMesh->numVerts       = guiMeshSurf->numVerts;

            subMesh->vertexCache    = (BufferCache *)frameData.ClearedAlloc(sizeof(BufferCache));
            *(subMesh->vertexCache) = guiMeshSurf->vertexCache;

            subMesh->indexCache     = (BufferCache *)frameData.ClearedAlloc(sizeof(BufferCache));
            *(subMesh->indexCache)  = guiMeshSurf->indexCache;

            AddDrawSurf(view, viewEntity, guiMeshSurf->material, subMesh, flags);
        }
    }
}

void RenderWorld::AddSkyBoxMeshes(view_t *view) {
    if (view->def->parms.clearMethod != SceneView::SkyboxClear) {
        return;
    }

    // skybox entity parameters
    SceneEntity::Parms entityParms;
    memset(&entityParms, 0, sizeof(entityParms));
    entityParms.origin = view->def->parms.origin;
    entityParms.scale = Vec3::one;
    entityParms.axis = Mat3::identity;
    entityParms.materialParms[SceneEntity::TimeScaleParm] = 1.0f;

    static SceneEntity sceneEntity;
    new (&sceneEntity) SceneEntity();
    sceneEntity.Update(&entityParms);

    // skybox view entity
    viewEntity_t *viewEntity = RegisterViewEntity(view, &sceneEntity);

    if (view->def->parms.orthogonal) {
        Mat4 projMatrix;
        R_SetPerspectiveProjectionMatrix(45, 45, 1, 1000, false, projMatrix);
        viewEntity->modelViewMatrix = view->def->viewMatrix * sceneEntity.GetModelMatrix();
        viewEntity->modelViewProjMatrix = projMatrix * viewEntity->modelViewMatrix;
    } else {
        viewEntity->modelViewMatrix = view->def->viewMatrix * sceneEntity.GetModelMatrix();
        viewEntity->modelViewProjMatrix = view->def->viewProjMatrix * sceneEntity.GetModelMatrix();
    }

    MeshSurf *meshSurf = meshManager.defaultBoxMesh->GetSurface(0);
    AddDrawSurf(view, viewEntity, skyboxMaterial, meshSurf->subMesh, DrawSurf::AmbientVisible);
}

// static mesh 들을 viewLight 의 litSurfs/shadowCasterSurfs 리스트에 담는다.
void RenderWorld::AddStaticMeshesForLights(view_t *view) {
    viewLight_t *viewLight = view->viewLights;

    // Called for static mesh surfaces intersecting with each light volume
    // Returns true if it want to proceed next query
    auto addStaticMeshSurfsForLights = [this, view, &viewLight](int32_t proxyId) -> bool {
        const DbvtProxy *proxy = (const DbvtProxy *)staticMeshDbvt.GetUserData(proxyId);
        const SceneEntity *proxyEntity = proxy->sceneEntity;

        MeshSurf *surf = proxy->mesh->GetSurface(proxy->meshSurfIndex);

        if (!surf) {
            return true;
        }

        // Skip first person view only entity in subView 
        if (proxyEntity->parms.firstPersonOnly && view->isSubview) {
            return true;
        }

        // Skip 3rd person view only entity in subView
        if (proxyEntity->parms.thirdPersonOnly && !view->isSubview) {
            return true;
        }

        if (!(BIT(viewLight->def->parms.layer) & view->def->parms.layerMask)) {
            return true;
        }

        // Skip if the entity is farther than maximum visible distance
        if (proxyEntity->parms.origin.DistanceSqr(view->def->parms.origin) > proxyEntity->parms.maxVisDist * proxyEntity->parms.maxVisDist) {
            return true;
        }

        const Material *material = proxyEntity->parms.materials[surf->materialIndex];

        // Is surface visible for this frame ?
        if (surf->viewCount == this->viewCount) {
            if ((surf->drawSurf->flags & DrawSurf::AmbientVisible) && material->IsLitSurface()) {
                drawSurfNode_t *drawSurfNode = (drawSurfNode_t *)frameData.Alloc(sizeof(drawSurfNode_t));
                drawSurfNode->drawSurf = surf->drawSurf;
                drawSurfNode->next = viewLight->litSurfs;

                viewLight->litSurfs = drawSurfNode;
                viewLight->litSurfsAABB.AddAABB(proxy->aabb);
            }
        }
            
        if (proxyEntity->parms.castShadows && material->IsShadowCaster()) {
            OBB surfBounds = OBB(surf->subMesh->GetAABB() * proxyEntity->parms.scale, proxyEntity->parms.origin, proxyEntity->parms.axis);
            if (viewLight->def->CullShadowCasterOBB(surfBounds, view->def->frustum, view->aabb)) {
                return true;
            }

            // This surface is not visible but shadow might be visible as a shadow caster.
            if (surf->viewCount != this->viewCount) {
                // Register a viewEntity used only for shadow caster
                viewEntity_t *shadowViewEntity = RegisterViewEntity(view, proxy->sceneEntity);
                shadowViewEntity->shadowVisible = true;

                AddDrawSurf(view, shadowViewEntity, material, surf->subMesh, 0);

                surf->viewCount = this->viewCount;
                surf->drawSurf = view->drawSurfs[view->numDrawSurfs - 1];
            }

            drawSurfNode_t *drawSurfNode = (drawSurfNode_t *)frameData.Alloc(sizeof(drawSurfNode_t));
            drawSurfNode->drawSurf = surf->drawSurf;
            drawSurfNode->next = viewLight->shadowCasterSurfs;

            viewLight->shadowCasterSurfs = drawSurfNode;
            viewLight->shadowCasterAABB.AddAABB(proxy->aabb);
        }

        return true;
    };

    while (viewLight) {
        const SceneLight *sceneLight = viewLight->def;

        switch (sceneLight->parms.type) {
        case SceneLight::DirectionalLight:
            staticMeshDbvt.Query(sceneLight->obb, addStaticMeshSurfsForLights);
            break;
        case SceneLight::PointLight:
            if (sceneLight->IsRadiusUniform()) {
                staticMeshDbvt.Query(Sphere(sceneLight->GetOrigin(), sceneLight->GetRadius()[0]), addStaticMeshSurfsForLights);
            } else {
                staticMeshDbvt.Query(sceneLight->obb, addStaticMeshSurfsForLights);
            }
            break;
        case SceneLight::SpotLight:
            staticMeshDbvt.Query(sceneLight->frustum, addStaticMeshSurfsForLights);
            break;
        default:
            break;
       }

       viewLight = viewLight->next;
    }
}

// skinned mesh 들을 viewLight 의 litSurfs/shadowCasterSurfs 리스트에 담는다.
void RenderWorld::AddSkinnedMeshesForLights(view_t *view) {
    viewLight_t *viewLight = view->viewLights;

    // Called for entities intersecting with each light volume
    // Returns true if it want to proceed next query
    auto addShadowCasterEntities = [this, view, &viewLight](int32_t proxyId) -> bool {
        const DbvtProxy *proxy = (const DbvtProxy *)this->entityDbvt.GetUserData(proxyId);
        const SceneEntity *proxyEntity = proxy->sceneEntity;
            
        // sceneEntity 가 없다면 mesh 가 아님
        if (!proxyEntity) {
            return true;
        }

        if (!proxyEntity->parms.joints) {
            return true;
        }

        // Skip first person view only entity in subView 
        if (proxyEntity->parms.firstPersonOnly && view->isSubview) {
            return true;
        }

        // Skip 3rd person view only entity in subView
        if (proxyEntity->parms.thirdPersonOnly && !view->isSubview) {
            return true;
        }

        if (!(BIT(viewLight->def->parms.layer) & view->def->parms.layerMask)) {
            return true;
        }

        for (int surfaceIndex = 0; surfaceIndex < proxyEntity->parms.mesh->NumSurfaces(); surfaceIndex++) {
            MeshSurf *surf = proxyEntity->parms.mesh->GetSurface(surfaceIndex);

            const Material *material = proxyEntity->parms.materials[surf->materialIndex];

            // 이미 ambient visible surf 로 등록되었고, lighting 이 필요한 surf 라면 litSurfs 리스트에 추가한다.
            if (surf->viewCount == this->viewCount && surf->drawSurf->flags & DrawSurf::AmbientVisible && material->IsLitSurface()) {
                drawSurfNode_t *drawSurfNode = (drawSurfNode_t *)frameData.Alloc(sizeof(drawSurfNode_t));
                drawSurfNode->drawSurf = surf->drawSurf;
                drawSurfNode->next = viewLight->litSurfs;

                viewLight->litSurfs = drawSurfNode;
            }
        }

        if (proxyEntity->viewEntity && proxyEntity->viewEntity->ambientVisible) {
            viewLight->litSurfsAABB.AddAABB(proxy->aabb);
        }

        if (!proxyEntity->parms.castShadows) {
            return true;
        }

        OBB obb = OBB(proxyEntity->GetAABB(), proxyEntity->parms.origin, proxyEntity->parms.axis);
        if (viewLight->def->CullShadowCasterOBB(obb, view->def->frustum, view->aabb)) {
            return true;
        }

        viewEntity_t *shadowViewEntity = nullptr;
            
        for (int surfaceIndex = 0; surfaceIndex < proxyEntity->parms.mesh->NumSurfaces(); surfaceIndex++) {
            MeshSurf *surf = proxyEntity->parms.mesh->GetSurface(surfaceIndex);

            const Material *material = proxyEntity->parms.materials[surf->materialIndex];

            if (material->IsShadowCaster()) {
                if (surf->viewCount != this->viewCount) {
                    // ambient visible 하지 않으므로 shadow 용 viewEntity 를 등록해준다.
                    if (!shadowViewEntity) {
                        shadowViewEntity = RegisterViewEntity(view, proxy->sceneEntity);
                        shadowViewEntity->shadowVisible = true;
                    }

                    if (shadowViewEntity->def->parms.skeleton && shadowViewEntity->def->parms.joints) {
                        shadowViewEntity->def->parms.mesh->UpdateSkinningJointCache(shadowViewEntity->def->parms.skeleton, shadowViewEntity->def->parms.joints);
                    }

                    // drawSurf for shadow
                    AddDrawSurf(view, shadowViewEntity, material, surf->subMesh, 0);

                    surf->viewCount = this->viewCount;
                    surf->drawSurf = view->drawSurfs[view->numDrawSurfs - 1];
                }

                drawSurfNode_t *drawSurfNode = (drawSurfNode_t *)frameData.Alloc(sizeof(drawSurfNode_t));
                drawSurfNode->drawSurf = surf->drawSurf;
                drawSurfNode->next = viewLight->shadowCasterSurfs;
                
                viewLight->shadowCasterSurfs = drawSurfNode;
            }
        }

        viewLight->shadowCasterAABB.AddAABB(proxy->aabb);

        return true;
    };
    
    while (viewLight) {
        const SceneLight *sceneLight = viewLight->def;

        switch (sceneLight->parms.type) {
        case SceneLight::DirectionalLight:
            entityDbvt.Query(sceneLight->obb, addShadowCasterEntities);
            break;
        case SceneLight::PointLight:
            if (sceneLight->IsRadiusUniform()) {
                entityDbvt.Query(Sphere(sceneLight->GetOrigin(), sceneLight->GetRadius()[0]), addShadowCasterEntities);
            } else {
                entityDbvt.Query(sceneLight->obb, addShadowCasterEntities);
            }
            break;
        case SceneLight::SpotLight:
            entityDbvt.Query(sceneLight->frustum, addShadowCasterEntities);
            break;
        default:
            break;
        }

        viewLight = viewLight->next;
    }
}

void RenderWorld::OptimizeLights(view_t *view) {
    viewLight_t *prevLight = nullptr;

    // Iterate over light linked list in view
    for (viewLight_t *light = view->viewLights; light; light = light->next) {
        const AABB lightAABB = light->def->GetAABB();
        
        // Compute effective AABB
        light->litSurfsAABB.IntersectSelf(lightAABB);
        light->shadowCasterAABB.IntersectSelf(lightAABB);
        
        if (light->def->parms.isPrimaryLight) {
            view->primaryLight = light;
            continue;
        }

        Rect screenClipRect;
        if (!view->def->GetClipRectFromAABB(light->litSurfsAABB, screenClipRect)) {
            if (prevLight) {
                prevLight->next = light->next;
            } else {
                view->viewLights = light->next;
            }
            continue;
        }

        Rect visScissorRect = screenClipRect;
        visScissorRect.y = renderSystem.currentContext->GetRenderingHeight() - (screenClipRect.y + screenClipRect.h);
        light->scissorRect.IntersectSelf(visScissorRect);

        if (light->scissorRect.IsEmpty()) {
            if (prevLight) {
                prevLight->next = light->next;
            } else {
                view->viewLights = light->next;
            }
            continue;
        }

        prevLight = light;
    }
}

static int BE_CDECL _CompareDrawSurf(const void *elem1, const void *elem2) {
    const uint64_t sortKey1 = (*(DrawSurf **)elem1)->sortKey;
    const uint64_t sortKey2 = (*(DrawSurf **)elem2)->sortKey;

    if (sortKey1 < sortKey2) {
        return -1;
    }
    if (sortKey1 > sortKey2) {
        return 1;
    }
    return 0;
}

void RenderWorld::SortDrawSurfs(view_t *view) {
    qsort(view->drawSurfs, view->numDrawSurfs, sizeof(DrawSurf *), _CompareDrawSurf);
}

void RenderWorld::RenderView(view_t *view) {
    // view frustum 을 entity dynamic bounding volume tree 에 query 해서 빠르게 sceneLight, sceneEntity 를 찾는다.
    // 찾은 def 들은 각각 viewLights, viewEntities 를 생성하며 view 에 등록
    // sceneEntity 와 sceneLight 의 pointer 에도 연결 (아래 단계에서 다시 한번 dbvt 를 seaching 할때 이미 등록된 viewLights/viewEntities 를 한번에 찾기위해)
    FindViewLightsAndEntities(view);

    // staticDBVT 를 query 해서 찾은 static mesh surface 를 drawSurf 에 등록
    AddStaticMeshes(view);

    // viewEntities 를 iteration 하며 skinned mesh surface 를 drawSurf 에 등록
    AddSkinnedMeshes(view);

    AddParticleMeshes(view);

    AddTextMeshes(view);

    AddSkyBoxMeshes(view);

    // 등록된 모든 ambient visible 한 drawSurfs 들을 sorting
    SortDrawSurfs(view);
    
    /*if (!(view->def->parms.flags & NoSubViews)) {
        for (int i = 0; i < view->numDrawSurfs; i++) {
            DrawSurf *drawSurf = view->drawSurfs[i];

            if (!(drawSurf->flags & DrawSurf::AmbientVisible)) {
                continue;
            }

            if (drawSurf->material->GetSort() > SubViewSort) {
                break;
            }

            if (drawSurf->material->GetSort() == BadSort) {
                BE_ERRLOG(L"Material '%hs' with sort == BadSort\n", drawSurf->material->GetName());
            }

            RenderSubView(drawSurf->entity, drawSurf, drawSurf->material);

            if (r_subViewOnly.GetBool()) {
                break;
            }
        }
    }*/
    
    // light 의 bounding volume 으로 staticDBVT 를 query 하여,
    // ambient visible 여부, shadow bounding volume 의 visible 여부
    // ambient visible 이라면 viewLight 의 litSurf 에 등록
    // shadow visible 이라면 shadowCasterSurf 에 등록
    AddStaticMeshesForLights(view);

    // 바로 윗 단계와 비슷하나 entity 단위로 컬링하고 skinned mesh 의 surf 를 한꺼번에 등록
    AddSkinnedMeshesForLights(view);

    OptimizeLights(view);

    renderSystem.CmdDrawView(view);
}

void RenderWorld::RenderSubView(viewEntity_t *viewEntity, const DrawSurf *drawSurf, const Material *material) {
}

void RenderWorld::AddDrawSurf(view_t *view, viewEntity_t *viewEntity, const Material *material, SubMesh *subMesh, int flags) {
    if (view->numDrawSurfs + 1 > view->maxDrawSurfs) {
        BE_WARNLOG(L"RenderWorld::AddDrawSurf: not enough renderable surfaces\n");
        return;
    }

    const Material *realMaterial = material;
    if (!realMaterial) {
        realMaterial = materialManager.defaultMaterial;
    }

    //if (viewEntity->def->parms.customSkin) {
    //	realMaterial = (viewEntity->def->parms.customSkin)->RemapMaterialBySkin(material);
    //}

    /*float *outputValues = (float *)frameData.Alloc(realMaterial->GetExprChunk()->NumRegisters() * sizeof(float));
    float localParms[MAX_EXPR_LOCALPARMS];
    localParms[0] = view->def->parms.time * 0.001f;
    memcpy(&localParms[1], viewEntity->def->parms.materialParms, sizeof(viewEntity->def->parms.materialParms));

    realMaterial->GetExprChunk()->Evaluate(localParms, outputValues);*/

    if (!bufferCacheManager.IsCached(subMesh->vertexCache)) {
        if (subMesh->GetType() == Mesh::ReferenceMesh || 
            subMesh->GetType() == Mesh::StaticMesh || 
            subMesh->GetType() == Mesh::SkinnedMesh) {
            subMesh->CacheStaticDataToGpu();
        } else {
            subMesh->CacheDynamicDataToGpu(viewEntity->def->parms.joints, realMaterial);
        }
    }

    DrawSurf *drawSurf = (DrawSurf *)frameData.ClearedAlloc(sizeof(DrawSurf));
    drawSurf->space             = viewEntity;
    drawSurf->material          = realMaterial;
    drawSurf->materialRegisters = nullptr;//outputValues;
    drawSurf->subMesh           = subMesh;
    drawSurf->flags             = flags;

    uint64_t materialSort = realMaterial->GetSort();
    uint64_t entityIndex = viewEntity->def->index;
    uint64_t materialIndex = materialManager.GetIndexByMaterial(realMaterial);
    uint64_t depthDist = 0;
    
    if (materialSort == Material::TranslucentSort || materialSort == Material::OverlaySort) {
        float depthMin = 0.0f;
        float depthMax = 1.0f;

        view->def->GetDepthBoundsFromAABB(subMesh->GetAABB(), viewEntity->modelViewProjMatrix, &depthMin, &depthMax);

        depthDist = Math::Ftoui16((1.0f - depthMin) * 0xFFFF);
    }

    //---------------------------------------------------
    // sortKey bits:
    // 0xFFFF000000000000 (0~65535) : material sort
    // 0x0000FFFF00000000 (0~65535) : depth dist
    // 0x00000000FFFF0000 (0~65535) : entity index
    // 0x000000000000FFFF (0~65535) : material index
    //---------------------------------------------------
    drawSurf->sortKey = ((materialSort << 48) | (depthDist << 32) | (entityIndex << 16) | materialIndex);
    
    view->drawSurfs[view->numDrawSurfs++] = drawSurf;
}

BE_NAMESPACE_END
