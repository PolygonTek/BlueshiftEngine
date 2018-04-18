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

// sceneObject 로 부터 visibleObject 를 등록, 같은 view 에 여러번 등록 방지
VisibleObject *RenderWorld::RegisterVisibleObject(VisibleView *view, SceneObject *sceneObject) {
    if (sceneObject->viewCount == viewCount) {
        // already registered for this frame
        return sceneObject->visibleObject;
    }

    VisibleObject *visibleObject = (VisibleObject *)frameData.ClearedAlloc(sizeof(*visibleObject));
    visibleObject->def = sceneObject;

    // view 에 linked list 로 연결
    visibleObject->next = view->visibleObjects;
    view->visibleObjects = visibleObject;

    // visibleObject 를 sceneObject 에 연결
    sceneObject->visibleObject = visibleObject;

    // RenderView 에서 1 씩 증가되는 viewCount
    sceneObject->viewCount = viewCount;

    return visibleObject;
}

// sceneLight 로 부터 visibleLight 를 등록, 같은 view 에 여러번 등록 방지
VisibleLight *RenderWorld::RegisterVisibleLight(VisibleView *view, SceneLight *sceneLight) {
    if (sceneLight->viewCount == viewCount) {
        // already registered for this frame
        return sceneLight->visibleLight;
    }

    VisibleLight *visibleLight = (VisibleLight *)frameData.ClearedAlloc(sizeof(*visibleLight));
    visibleLight->def = sceneLight;

    // view 에 linked list 로 연결
    visibleLight->next = view->visibleLights;
    view->visibleLights = visibleLight;

    // visibleLight 를 sceneLight 에 연결
    sceneLight->visibleLight = visibleLight;

    // RenderView 에서 1 씩 증가되는 viewCount
    sceneLight->viewCount = viewCount;

    return visibleLight;
}

// visibleLight 와 visibleObject 들을 등록한다.
void RenderWorld::FindVisibleLightsAndObjects(VisibleView *view) {
    viewCount++;

    view->aabb.Clear();
    view->visibleLights = nullptr;
    view->visibleObjects = nullptr;

    // Called for each lights intersecting with view frustum 
    // Returns true if it want to proceed next query
    auto addVisibleLights = [this, view](int32_t proxyId) -> bool {
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

        VisibleLight *visibleLight = RegisterVisibleLight(view, sceneLight);
        visibleLight->scissorRect = screenClipRect;
        // glScissor 의 x, y 좌표는 lower left corner 이므로 y 좌표를 밑에서 증가되도록 뒤집는다.
        visibleLight->scissorRect.y = renderSystem.currentContext->GetRenderingHeight() - (screenClipRect.y + screenClipRect.h);

        visibleLight->litSurfsAABB.Clear();
        visibleLight->shadowCasterAABB.Clear();

        return true;
    };

    // Called for each entities intersecting with view frustum 
    // Returns true if it want to proceed next query
    auto addVisibleObjects = [this, view](int32_t proxyId) -> bool {
        DbvtProxy *proxy = (DbvtProxy *)entityDbvt.GetUserData(proxyId);
        SceneObject *sceneObject = proxy->sceneObject;

        if (!sceneObject) {
            return true;
        }

        if (!(BIT(sceneObject->parms.layer) & view->def->parms.layerMask)) {
            return true;
        }

        // Skip first person view only entity in subView 
        if (sceneObject->parms.firstPersonOnly && view->isSubview) {
            return true;
        }

        // Skip 3rd person view only entity in subView
        if (sceneObject->parms.thirdPersonOnly && !view->isSubview) {
            return true;
        }

        // Skip if a entity is farther than maximum visible distance
        if (sceneObject->parms.origin.DistanceSqr(view->def->parms.origin) > sceneObject->parms.maxVisDist * sceneObject->parms.maxVisDist) {
            return true;
        }

        VisibleObject *visibleObject = RegisterVisibleObject(view, sceneObject);

        visibleObject->ambientVisible = true;

        visibleObject->modelViewMatrix = view->def->viewMatrix * sceneObject->GetModelMatrix();
        visibleObject->modelViewProjMatrix = view->def->viewProjMatrix * sceneObject->GetModelMatrix();

        if (sceneObject->parms.billboard) {
            Mat3 inverse = (view->def->viewMatrix.ToMat3() * sceneObject->GetModelMatrix().ToMat3()).Inverse();
            //inverse = inverse * Mat3(0, 0, 1, 1, 0, 0, 0, 1, 0);
            Swap(inverse[0], inverse[2]);
            Swap(inverse[1], inverse[2]);

            Mat3 billboardMatrix = inverse * Mat3::FromScale(sceneObject->parms.scale);
            visibleObject->modelViewMatrix *= billboardMatrix;
            visibleObject->modelViewProjMatrix *= billboardMatrix;
        }

        view->aabb.AddAABB(proxy->aabb);

        if (r_showAABB.GetInteger() > 0) {
            SetDebugColor(Color4::blue, Color4::zero);
            DebugAABB(proxy->aabb, 1, true, r_showAABB.GetInteger() == 1 ? true : false);
        }

        if (visibleObject->def->parms.numJoints > 0 && r_showSkeleton.GetInteger() > 0) {
            DebugJoints(visibleObject->def, r_showSkeleton.GetInteger() == 2, view->def->parms.axis);
        }

        return true;
    };

    if (view->def->parms.orthogonal) {
        lightDbvt.Query(view->def->box, addVisibleLights);
        entityDbvt.Query(view->def->box, addVisibleObjects);
    } else {
        lightDbvt.Query(view->def->frustum, addVisibleLights);
        entityDbvt.Query(view->def->frustum, addVisibleObjects);
    }
}

// static mesh 들을 ambient drawSurfs 에 담는다.
void RenderWorld::AddStaticMeshes(VisibleView *view) {
    // Called for each static mesh surfaces intersecting with view frustum 
    // Returns true if it want to proceed next query
    auto addStaticMeshSurfs = [this, view](int32_t proxyId) -> bool {
        DbvtProxy *proxy = (DbvtProxy *)staticMeshDbvt.GetUserData(proxyId);
        MeshSurf *surf = proxy->mesh->GetSurface(proxy->meshSurfIndex);

        // surf 가 없다면 static mesh 가 아님
        if (!surf) {
            return true;
        }

        if (proxy->sceneObject->viewCount != this->viewCount) {
            return true;
        }

        /*if (proxy->lodGroup >= 0) {
            // Compute LOD value [0, 1]
            AABB aabb = surf->subMesh->GetAABB() * proxy->sceneObject->parms.scale;
            Vec3 size = aabb[1] - aabb[0];
            float area = view->def->parms.axis[0].Abs().Dot(Vec3(size.y * size.z, size.z * size.x, size.x * size.y));
            float d = view->def->parms.origin.DistanceSqr(proxy->sceneObject->parms.origin);
            float lodValue = area / d;
        }*/

#if 0
        // More accurate OBB culling
        OBB obb = OBB(proxy->sceneObject->GetAABB(), proxy->sceneObject->parms.origin, proxy->sceneObject->parms.axis);
        if (view->def->frustum.CullOBB(obb)) {
            return true;
        }
#endif

        int flags = DrawSurf::AmbientVisible;
        if (proxy->sceneObject->parms.wireframeMode != SceneObject::WireframeMode::ShowNone || r_showWireframe.GetInteger() > 0) {
            flags |= DrawSurf::ShowWires;
        }

        VisibleObject *visibleObject = proxy->sceneObject->visibleObject;
        AddDrawSurf(view, visibleObject, visibleObject->def->parms.materials[surf->materialIndex], surf->subMesh, flags);

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
void RenderWorld::AddSkinnedMeshes(VisibleView *view) {
    for (VisibleObject *visibleObject = view->visibleObjects; visibleObject; visibleObject = visibleObject->next) {
        if (!visibleObject->ambientVisible) {
            continue;
        }

        const SceneObject::Parms &sceneObjectParms = visibleObject->def->parms;

        if (!sceneObjectParms.mesh || !sceneObjectParms.joints) {
            continue;
        }

#if 0
        // More accurate OBB culling
        OBB obb = OBB(visibleObject->def->GetAABB(), sceneObjectParms.origin, sceneObjectParms.axis);
        if (view->def->frustum.CullOBB(obb)) {
            continue;
        }
#endif

        int flags = DrawSurf::AmbientVisible;
        if (sceneObjectParms.wireframeMode != SceneObject::WireframeMode::ShowNone || r_showWireframe.GetInteger() > 0) {
            flags |= DrawSurf::ShowWires;
        }

        if (sceneObjectParms.skeleton && sceneObjectParms.joints) {
            // Update skinning joint cache for GPU skinning
            sceneObjectParms.mesh->UpdateSkinningJointCache(sceneObjectParms.skeleton, sceneObjectParms.joints);
        }

        for (int surfaceIndex = 0; surfaceIndex < sceneObjectParms.mesh->NumSurfaces(); surfaceIndex++) {
            MeshSurf *surf = sceneObjectParms.mesh->GetSurface(surfaceIndex);

            AddDrawSurf(view, visibleObject, sceneObjectParms.materials[surf->materialIndex], surf->subMesh, flags);

            surf->viewCount = viewCount;
            surf->drawSurf = view->drawSurfs[view->numDrawSurfs - 1];
        }
    }
}

void RenderWorld::AddParticleMeshes(VisibleView *view) {
    for (VisibleObject *visibleObject = view->visibleObjects; visibleObject; visibleObject = visibleObject->next) {
        if (!visibleObject->ambientVisible) {
            continue;
        }

        const SceneObject::Parms &sceneObjectParms = visibleObject->def->parms;

        if (!sceneObjectParms.particleSystem) {
            continue;
        }

        int flags = DrawSurf::AmbientVisible | DrawSurf::SkipSelection;
        if (sceneObjectParms.wireframeMode != SceneObject::WireframeMode::ShowNone || r_showWireframe.GetInteger() > 0) {
            flags |= DrawSurf::ShowWires;
        }

        particleMesh.Clear();
        particleMesh.Draw(sceneObjectParms.particleSystem, sceneObjectParms.stageParticles, visibleObject->def, view->def);
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

            AddDrawSurf(view, visibleObject, prtMeshSurf->material, subMesh, flags);
        }
    }
}

void RenderWorld::AddTextMeshes(VisibleView *view) {
    for (VisibleObject *visibleObject = view->visibleObjects; visibleObject; visibleObject = visibleObject->next) {
        if (!visibleObject->ambientVisible) {
            continue;
        }

        const SceneObject::Parms &sceneObjectParms = visibleObject->def->parms;

        if (sceneObjectParms.text.IsEmpty()) {
            continue;
        }

        int flags = DrawSurf::AmbientVisible;
        if (sceneObjectParms.wireframeMode != SceneObject::WireframeMode::ShowNone || r_showWireframe.GetInteger() > 0) {
            flags |= DrawSurf::ShowWires;
        }

        textMesh.Clear();
        textMesh.SetColor(Color4(&sceneObjectParms.materialParms[SceneObject::RedParm]));
        textMesh.Draw(sceneObjectParms.font, sceneObjectParms.textAnchor, sceneObjectParms.textAlignment, sceneObjectParms.lineSpacing, sceneObjectParms.textScale, sceneObjectParms.text);
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

            AddDrawSurf(view, visibleObject, guiMeshSurf->material, subMesh, flags);
        }
    }
}

void RenderWorld::AddSkyBoxMeshes(VisibleView *view) {
    if (view->def->parms.clearMethod != SceneView::SkyboxClear) {
        return;
    }

    // skybox entity parameters
    SceneObject::Parms sceneObjectParms;
    memset(&sceneObjectParms, 0, sizeof(sceneObjectParms));
    sceneObjectParms.origin = view->def->parms.origin;
    sceneObjectParms.scale = Vec3::one;
    sceneObjectParms.axis = Mat3::identity;
    sceneObjectParms.materialParms[SceneObject::TimeScaleParm] = 1.0f;

    static SceneObject sceneObject;
    new (&sceneObject) SceneObject();
    sceneObject.Update(&sceneObjectParms);

    // skybox view entity
    VisibleObject *visibleObject = RegisterVisibleObject(view, &sceneObject);

    if (view->def->parms.orthogonal) {
        Mat4 projMatrix;
        R_SetPerspectiveProjectionMatrix(45, 45, 1, 1000, false, projMatrix);
        visibleObject->modelViewMatrix = view->def->viewMatrix * sceneObject.GetModelMatrix();
        visibleObject->modelViewProjMatrix = projMatrix * visibleObject->modelViewMatrix;
    } else {
        visibleObject->modelViewMatrix = view->def->viewMatrix * sceneObject.GetModelMatrix();
        visibleObject->modelViewProjMatrix = view->def->viewProjMatrix * sceneObject.GetModelMatrix();
    }

    MeshSurf *meshSurf = meshManager.defaultBoxMesh->GetSurface(0);
    AddDrawSurf(view, visibleObject, skyboxMaterial, meshSurf->subMesh, DrawSurf::AmbientVisible);
}

// static mesh 들을 visibleLight 의 litSurfs/shadowCasterSurfs 리스트에 담는다.
void RenderWorld::AddStaticMeshesForLights(VisibleView *view) {
    VisibleLight *visibleLight = view->visibleLights;

    // Called for static mesh surfaces intersecting with each light volume
    // Returns true if it want to proceed next query
    auto addStaticMeshSurfsForLights = [this, view, &visibleLight](int32_t proxyId) -> bool {
        const DbvtProxy *proxy = (const DbvtProxy *)staticMeshDbvt.GetUserData(proxyId);
        const SceneObject *proxyObject = proxy->sceneObject;

        MeshSurf *surf = proxy->mesh->GetSurface(proxy->meshSurfIndex);

        if (!surf) {
            return true;
        }

        // Skip first person view only entity in subView 
        if (proxyObject->parms.firstPersonOnly && view->isSubview) {
            return true;
        }

        // Skip 3rd person view only entity in subView
        if (proxyObject->parms.thirdPersonOnly && !view->isSubview) {
            return true;
        }

        if (!(BIT(visibleLight->def->parms.layer) & view->def->parms.layerMask)) {
            return true;
        }

        // Skip if the entity is farther than maximum visible distance
        if (proxyObject->parms.origin.DistanceSqr(view->def->parms.origin) > proxyObject->parms.maxVisDist * proxyObject->parms.maxVisDist) {
            return true;
        }

        const Material *material = proxyObject->parms.materials[surf->materialIndex];

        // Is surface visible for this frame ?
        if (surf->viewCount == this->viewCount) {
            if ((surf->drawSurf->flags & DrawSurf::AmbientVisible) && material->IsLitSurface()) {
                drawSurfNode_t *drawSurfNode = (drawSurfNode_t *)frameData.Alloc(sizeof(drawSurfNode_t));
                drawSurfNode->drawSurf = surf->drawSurf;
                drawSurfNode->next = visibleLight->litSurfs;

                visibleLight->litSurfs = drawSurfNode;
                visibleLight->litSurfsAABB.AddAABB(proxy->aabb);
            }
        }

        if (proxyObject->parms.castShadows && material->IsShadowCaster()) {
            OBB surfBounds = OBB(surf->subMesh->GetAABB() * proxyObject->parms.scale, proxyObject->parms.origin, proxyObject->parms.axis);
            if (visibleLight->def->CullShadowCasterOBB(surfBounds, view->def->frustum, view->aabb)) {
                return true;
            }

            // This surface is not visible but shadow might be visible as a shadow caster.
            if (surf->viewCount != this->viewCount) {
                // Register a visibleObject used only for shadow caster
                VisibleObject *shadowVisibleObject = RegisterVisibleObject(view, proxy->sceneObject);
                shadowVisibleObject->shadowVisible = true;

                AddDrawSurf(view, shadowVisibleObject, material, surf->subMesh, 0);

                surf->viewCount = this->viewCount;
                surf->drawSurf = view->drawSurfs[view->numDrawSurfs - 1];
            }

            drawSurfNode_t *drawSurfNode = (drawSurfNode_t *)frameData.Alloc(sizeof(drawSurfNode_t));
            drawSurfNode->drawSurf = surf->drawSurf;
            drawSurfNode->next = visibleLight->shadowCasterSurfs;

            visibleLight->shadowCasterSurfs = drawSurfNode;
            visibleLight->shadowCasterAABB.AddAABB(proxy->aabb);
        }

        return true;
    };

    while (visibleLight) {
        const SceneLight *sceneLight = visibleLight->def;

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

       visibleLight = visibleLight->next;
    }
}

// skinned mesh 들을 visibleLight 의 litSurfs/shadowCasterSurfs 리스트에 담는다.
void RenderWorld::AddSkinnedMeshesForLights(VisibleView *view) {
    VisibleLight *visibleLight = view->visibleLights;

    // Called for entities intersecting with each light volume
    // Returns true if it want to proceed next query
    auto addShadowCasterEntities = [this, view, &visibleLight](int32_t proxyId) -> bool {
        const DbvtProxy *proxy = (const DbvtProxy *)this->entityDbvt.GetUserData(proxyId);
        const SceneObject *proxyObject = proxy->sceneObject;
            
        // sceneObject 가 없다면 mesh 가 아님
        if (!proxyObject) {
            return true;
        }

        if (!proxyObject->parms.joints) {
            return true;
        }

        // Skip first person view only entity in subView 
        if (proxyObject->parms.firstPersonOnly && view->isSubview) {
            return true;
        }

        // Skip 3rd person view only entity in subView
        if (proxyObject->parms.thirdPersonOnly && !view->isSubview) {
            return true;
        }

        if (!(BIT(visibleLight->def->parms.layer) & view->def->parms.layerMask)) {
            return true;
        }

        for (int surfaceIndex = 0; surfaceIndex < proxyObject->parms.mesh->NumSurfaces(); surfaceIndex++) {
            MeshSurf *surf = proxyObject->parms.mesh->GetSurface(surfaceIndex);

            const Material *material = proxyObject->parms.materials[surf->materialIndex];

            // 이미 ambient visible surf 로 등록되었고, lighting 이 필요한 surf 라면 litSurfs 리스트에 추가한다.
            if (surf->viewCount == this->viewCount && surf->drawSurf->flags & DrawSurf::AmbientVisible && material->IsLitSurface()) {
                drawSurfNode_t *drawSurfNode = (drawSurfNode_t *)frameData.Alloc(sizeof(drawSurfNode_t));
                drawSurfNode->drawSurf = surf->drawSurf;
                drawSurfNode->next = visibleLight->litSurfs;

                visibleLight->litSurfs = drawSurfNode;
            }
        }

        if (proxyObject->visibleObject && proxyObject->visibleObject->ambientVisible) {
            visibleLight->litSurfsAABB.AddAABB(proxy->aabb);
        }

        if (!proxyObject->parms.castShadows) {
            return true;
        }

        OBB obb = OBB(proxyObject->GetAABB(), proxyObject->parms.origin, proxyObject->parms.axis);
        if (visibleLight->def->CullShadowCasterOBB(obb, view->def->frustum, view->aabb)) {
            return true;
        }

        VisibleObject *shadowVisibleObject = nullptr;
            
        for (int surfaceIndex = 0; surfaceIndex < proxyObject->parms.mesh->NumSurfaces(); surfaceIndex++) {
            MeshSurf *surf = proxyObject->parms.mesh->GetSurface(surfaceIndex);

            const Material *material = proxyObject->parms.materials[surf->materialIndex];

            if (material->IsShadowCaster()) {
                if (surf->viewCount != this->viewCount) {
                    // ambient visible 하지 않으므로 shadow 용 visibleObject 를 등록해준다.
                    if (!shadowVisibleObject) {
                        shadowVisibleObject = RegisterVisibleObject(view, proxy->sceneObject);
                        shadowVisibleObject->shadowVisible = true;
                    }

                    if (shadowVisibleObject->def->parms.skeleton && shadowVisibleObject->def->parms.joints) {
                        shadowVisibleObject->def->parms.mesh->UpdateSkinningJointCache(shadowVisibleObject->def->parms.skeleton, shadowVisibleObject->def->parms.joints);
                    }

                    // drawSurf for shadow
                    AddDrawSurf(view, shadowVisibleObject, material, surf->subMesh, 0);

                    surf->viewCount = this->viewCount;
                    surf->drawSurf = view->drawSurfs[view->numDrawSurfs - 1];
                }

                drawSurfNode_t *drawSurfNode = (drawSurfNode_t *)frameData.Alloc(sizeof(drawSurfNode_t));
                drawSurfNode->drawSurf = surf->drawSurf;
                drawSurfNode->next = visibleLight->shadowCasterSurfs;
                
                visibleLight->shadowCasterSurfs = drawSurfNode;
            }
        }

        visibleLight->shadowCasterAABB.AddAABB(proxy->aabb);

        return true;
    };
    
    while (visibleLight) {
        const SceneLight *sceneLight = visibleLight->def;

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

        visibleLight = visibleLight->next;
    }
}

void RenderWorld::OptimizeLights(VisibleView *view) {
    VisibleLight *prevLight = nullptr;

    // Iterate over light linked list in view
    for (VisibleLight *light = view->visibleLights; light; light = light->next) {
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
                view->visibleLights = light->next;
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
                view->visibleLights = light->next;
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

void RenderWorld::SortDrawSurfs(VisibleView *view) {
    qsort(view->drawSurfs, view->numDrawSurfs, sizeof(DrawSurf *), _CompareDrawSurf);
}

void RenderWorld::RenderView(VisibleView *view) {
    // view frustum 을 entity dynamic bounding volume tree 에 query 해서 빠르게 sceneLight, sceneObject 를 찾는다.
    // 찾은 def 들은 각각 visibleLights, visibleObjects 를 생성하며 view 에 등록
    // sceneObject 와 sceneLight 의 pointer 에도 연결 (아래 단계에서 다시 한번 dbvt 를 seaching 할때 이미 등록된 visibleLights/visibleObjects 를 한번에 찾기위해)
    FindVisibleLightsAndObjects(view);

    // staticDBVT 를 query 해서 찾은 static mesh surface 를 drawSurf 에 등록
    AddStaticMeshes(view);

    // visibleObjects 를 iteration 하며 skinned mesh surface 를 drawSurf 에 등록
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
    // ambient visible 이라면 visibleLight 의 litSurf 에 등록
    // shadow visible 이라면 shadowCasterSurf 에 등록
    AddStaticMeshesForLights(view);

    // 바로 윗 단계와 비슷하나 entity 단위로 컬링하고 skinned mesh 의 surf 를 한꺼번에 등록
    AddSkinnedMeshesForLights(view);

    OptimizeLights(view);

    renderSystem.CmdDrawView(view);
}

void RenderWorld::RenderSubView(VisibleObject *visibleObject, const DrawSurf *drawSurf, const Material *material) {
}

void RenderWorld::AddDrawSurf(VisibleView *view, VisibleObject *visibleObject, const Material *material, SubMesh *subMesh, int flags) {
    if (view->numDrawSurfs + 1 > view->maxDrawSurfs) {
        BE_WARNLOG(L"RenderWorld::AddDrawSurf: not enough renderable surfaces\n");
        return;
    }

    const Material *realMaterial = material;
    if (!realMaterial) {
        realMaterial = materialManager.defaultMaterial;
    }

    //if (visibleObject->def->parms.customSkin) {
    //	realMaterial = (visibleObject->def->parms.customSkin)->RemapMaterialBySkin(material);
    //}

    /*float *outputValues = (float *)frameData.Alloc(realMaterial->GetExprChunk()->NumRegisters() * sizeof(float));
    float localParms[MAX_EXPR_LOCALPARMS];
    localParms[0] = view->def->parms.time * 0.001f;
    memcpy(&localParms[1], visibleObject->def->parms.materialParms, sizeof(visibleObject->def->parms.materialParms));

    realMaterial->GetExprChunk()->Evaluate(localParms, outputValues);*/

    if (!bufferCacheManager.IsCached(subMesh->vertexCache)) {
        if (subMesh->GetType() == Mesh::ReferenceMesh || 
            subMesh->GetType() == Mesh::StaticMesh || 
            subMesh->GetType() == Mesh::SkinnedMesh) {
            subMesh->CacheStaticDataToGpu();
        } else {
            subMesh->CacheDynamicDataToGpu(visibleObject->def->parms.joints, realMaterial);
        }
    }

    DrawSurf *drawSurf = (DrawSurf *)frameData.ClearedAlloc(sizeof(DrawSurf));
    drawSurf->space             = visibleObject;
    drawSurf->material          = realMaterial;
    drawSurf->materialRegisters = nullptr;//outputValues;
    drawSurf->subMesh           = subMesh;
    drawSurf->flags             = flags;

    uint64_t materialSort = realMaterial->GetSort();
    uint64_t objectIndex = visibleObject->def->index;
    uint64_t materialIndex = materialManager.GetIndexByMaterial(realMaterial);
    uint64_t depthDist = 0;
    
    if (materialSort == Material::TranslucentSort || materialSort == Material::OverlaySort) {
        float depthMin = 0.0f;
        float depthMax = 1.0f;

        view->def->GetDepthBoundsFromAABB(subMesh->GetAABB(), visibleObject->modelViewProjMatrix, &depthMin, &depthMax);

        depthDist = Math::Ftoui16((1.0f - depthMin) * 0xFFFF);
    }

    //---------------------------------------------------
    // sortKey bits:
    // 0xFFFF000000000000 (0~65535) : material sort
    // 0x0000FFFF00000000 (0~65535) : depth dist
    // 0x00000000FFFF0000 (0~65535) : object index
    // 0x000000000000FFFF (0~65535) : material index
    //---------------------------------------------------
    drawSurf->sortKey = ((materialSort << 48) | (depthDist << 32) | (objectIndex << 16) | materialIndex);
    
    view->drawSurfs[view->numDrawSurfs++] = drawSurf;
}

BE_NAMESPACE_END
