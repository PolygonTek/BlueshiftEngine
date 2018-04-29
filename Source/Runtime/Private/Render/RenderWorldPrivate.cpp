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

// renderObject 로부터 visibleObject 를 등록, 같은 view 에 여러번 등록 방지
VisibleObject *RenderWorld::RegisterVisibleObject(VisibleView *view, RenderObject *renderObject) {
    if (renderObject->viewCount == viewCount) {
        // already registered for this frame
        return renderObject->visibleObject;
    }

    VisibleObject *visibleObject = (VisibleObject *)frameData.ClearedAlloc(sizeof(*visibleObject));

    new (&visibleObject->node) LinkList<VisibleObject>();
    visibleObject->node.SetOwner(visibleObject);
    visibleObject->node.AddToEnd(view->visibleObjects);

    visibleObject->index = view->numVisibleObjects++;

    visibleObject->def = renderObject;

    // visibleObject 를 renderObject 에 연결
    renderObject->visibleObject = visibleObject;

    // RenderCamera() 에서 1 씩 증가되는 viewCount
    renderObject->viewCount = viewCount;

    return visibleObject;
}

// renderLight 로부터 visibleLight 를 등록, 같은 view 에 여러번 등록 방지
VisibleLight *RenderWorld::RegisterVisibleLight(VisibleView *view, RenderLight *renderLight) {
    if (renderLight->viewCount == viewCount) {
        // already registered for this frame
        return renderLight->visibleLight;
    }

    VisibleLight *visibleLight = (VisibleLight *)frameData.ClearedAlloc(sizeof(*visibleLight));

    new (&visibleLight->node) LinkList<VisibleLight>();
    visibleLight->node.SetOwner(visibleLight);
    visibleLight->node.AddToEnd(view->visibleLights);

    visibleLight->index = view->numVisibleLights++;

    visibleLight->def = renderLight;

    visibleLight->litSurfsAABB.Clear();
    visibleLight->shadowCasterAABB.Clear();

    // visibleLight 를 renderLight 에 연결
    renderLight->visibleLight = visibleLight;

    // RenderCamera() 에서 1 씩 증가되는 viewCount
    renderLight->viewCount = viewCount;

    return visibleLight;
}

// view volume 으로 visibleLight 와 visibleObject 들을 등록한다.
void RenderWorld::FindVisibleLightsAndObjects(VisibleView *view) {
    viewCount++;

    view->worldAABB.Clear();
    view->visibleLights.Clear();
    view->visibleObjects.Clear();

    // Called for each scene lights that intersects with view frustum.
    // Returns true if it want to proceed next query.
    auto addVisibleLights = [this, view](int32_t proxyId) -> bool {
        DbvtProxy *proxy = (DbvtProxy *)lightDbvt.GetUserData(proxyId);
        RenderLight *renderLight = proxy->renderLight;

        if (!renderLight) {
            return true;
        }

        if (!(BIT(renderLight->state.layer) & view->def->state.layerMask)) {
            return true;
        }

        if (!(renderLight->state.flags & RenderLight::TurnOnFlag)) {
            return true;
        }

        // Skip if a light is farther than maximum visible distance
        if (renderLight->state.origin.DistanceSqr(view->def->state.origin) > renderLight->state.maxVisDist * renderLight->state.maxVisDist) {
            return true;
        }

        // Cull exact light bounding volume
        if (view->def->state.orthogonal) {
            if (renderLight->Cull(view->def->box)) {
                return true;
            }
        } else {
            if (renderLight->Cull(view->def->frustum)) {
                return true;
            }
        }

        // Calculate light scissor rect
        Rect screenClipRect;
        if (!renderLight->ComputeScreenClipRect(view->def, screenClipRect)) {
            return true;
        }

        VisibleLight *visibleLight = RegisterVisibleLight(view, renderLight);
        visibleLight->scissorRect = screenClipRect;
        // glScissor 의 x, y 좌표는 lower left corner 이므로 y 좌표를 밑에서 증가되도록 뒤집는다.
        visibleLight->scissorRect.y = renderSystem.currentContext->GetRenderingHeight() - (screenClipRect.y + screenClipRect.h);

        return true;
    };

    // Called for each scene objects that intersects with view frustum.
    // Returns true if it want to proceed next query.
    auto addVisibleObjects = [this, view](int32_t proxyId) -> bool {
        DbvtProxy *proxy = (DbvtProxy *)objectDbvt.GetUserData(proxyId);
        RenderObject *renderObject = proxy->renderObject;

        if (!renderObject) {
            return true;
        }

        if (!(BIT(renderObject->state.layer) & view->def->state.layerMask)) {
            return true;
        }

        // Skip first person view only object in sub camera
        if ((renderObject->state.flags & RenderObject::FirstPersonOnlyFlag) && view->isSubview) {
            return true;
        }

        // Skip 3rd person view only object in sub camera
        if ((renderObject->state.flags & RenderObject::ThirdPersonOnlyFlag) && !view->isSubview) {
            return true;
        }

        // Skip if a entity is farther than maximum visible distance
        if (renderObject->state.origin.DistanceSqr(view->def->state.origin) > renderObject->state.maxVisDist * renderObject->state.maxVisDist) {
            return true;
        }

        // Register visible object form the render object
        VisibleObject *visibleObject = RegisterVisibleObject(view, renderObject);

        visibleObject->ambientVisible = true;

        visibleObject->modelViewMatrix = view->def->viewMatrix * renderObject->GetObjectToWorldMatrix();
        visibleObject->modelViewProjMatrix = view->def->viewProjMatrix * renderObject->GetObjectToWorldMatrix();

        if (renderObject->state.flags & RenderObject::BillboardFlag) {
            Mat3 inverse = (view->def->viewMatrix.ToMat3() * renderObject->GetObjectToWorldMatrix().ToMat3()).Inverse();
            //inverse = inverse * Mat3(0, 0, 1, 1, 0, 0, 0, 1, 0);
            Swap(inverse[0], inverse[2]);
            Swap(inverse[1], inverse[2]);

            Mat3 billboardMatrix = inverse * Mat3::FromScale(renderObject->state.scale);
            visibleObject->modelViewMatrix *= billboardMatrix;
            visibleObject->modelViewProjMatrix *= billboardMatrix;
        }

        view->worldAABB.AddAABB(proxy->worldAABB);

        if (r_showAABB.GetInteger() > 0) {
            SetDebugColor(Color4::blue, Color4::zero);
            DebugAABB(proxy->worldAABB, 1, true, r_showAABB.GetInteger() == 1 ? true : false);
        }

        if (visibleObject->def->state.numJoints > 0 && r_showSkeleton.GetInteger() > 0) {
            DebugJoints(visibleObject->def, r_showSkeleton.GetInteger() == 2, view->def->state.axis);
        }

        return true;
    };

    if (view->def->state.orthogonal) {
        lightDbvt.Query(view->def->box, addVisibleLights);
        objectDbvt.Query(view->def->box, addVisibleObjects);
    } else {
        lightDbvt.Query(view->def->frustum, addVisibleLights);
        objectDbvt.Query(view->def->frustum, addVisibleObjects);
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

        if (proxy->renderObject->viewCount != this->viewCount) {
            return true;
        }

        /*if (proxy->lodGroup >= 0) {
            // Compute LOD value [0, 1]
            AABB aabb = surf->subMesh->GetAABB() * proxy->renderObject->state.scale;
            Vec3 size = aabb[1] - aabb[0];
            float area = view->def->state.axis[0].Abs().Dot(Vec3(size.y * size.z, size.z * size.x, size.x * size.y));
            float d = view->def->state.origin.DistanceSqr(proxy->renderObject->state.origin);
            float lodValue = area / d;
        }*/

#if 0
        // More accurate OBB culling
        OBB obb = OBB(proxy->renderObject->GetAABB(), proxy->renderObject->state.origin, proxy->renderObject->state.axis);
        if (view->def->frustum.CullOBB(obb)) {
            return true;
        }
#endif

        int flags = DrawSurf::AmbientVisible;
        if (proxy->renderObject->state.wireframeMode != RenderObject::WireframeMode::ShowNone || r_showWireframe.GetInteger() > 0) {
            flags |= DrawSurf::ShowWires;
        }

        VisibleObject *visibleObject = proxy->renderObject->visibleObject;
        AddDrawSurf(view, nullptr, visibleObject, visibleObject->def->state.materials[surf->materialIndex], surf->subMesh, flags);

        view->numAmbientSurfs++;

        surf->viewCount = this->viewCount;
        surf->drawSurf = view->drawSurfs[view->numDrawSurfs - 1];
        //bool has = surf->drawSurf->material->IsLitSurface();

        if (r_showAABB.GetInteger() > 0) {
            SetDebugColor(Color4(1, 1, 1, 0.5), Color4::zero);
            DebugAABB(proxy->worldAABB, 1, true, r_showAABB.GetInteger() == 1 ? true : false);
        }

        return true;
    };

    if (view->def->state.orthogonal) {
        staticMeshDbvt.Query(view->def->box, addStaticMeshSurfs);
    } else {
        staticMeshDbvt.Query(view->def->frustum, addStaticMeshSurfs);
    }
}

// skinned mesh 들을 ambient drawSurfs 에 담는다. 
void RenderWorld::AddSkinnedMeshes(VisibleView *view) {
    for (VisibleObject *visibleObject = view->visibleObjects.Next(); visibleObject; visibleObject = visibleObject->node.Next()) {
        if (!visibleObject->ambientVisible) {
            continue;
        }

        const RenderObject::State &renderObjectDef = visibleObject->def->state;

        if (!renderObjectDef.mesh || !renderObjectDef.joints) {
            continue;
        }

#if 0
        // More accurate OBB culling
        OBB obb = OBB(visibleObject->def->GetAABB(), renderObjectDef.origin, renderObjectDef.axis);
        if (view->def->frustum.CullOBB(obb)) {
            continue;
        }
#endif

        int flags = DrawSurf::AmbientVisible;
        if (renderObjectDef.wireframeMode != RenderObject::WireframeMode::ShowNone || r_showWireframe.GetInteger() > 0) {
            flags |= DrawSurf::ShowWires;
        }

        if (renderObjectDef.skeleton && renderObjectDef.joints) {
            // Update skinning joint cache for GPU skinning
            renderObjectDef.mesh->UpdateSkinningJointCache(renderObjectDef.skeleton, renderObjectDef.joints);
        }

        for (int surfaceIndex = 0; surfaceIndex < renderObjectDef.mesh->NumSurfaces(); surfaceIndex++) {
            MeshSurf *surf = renderObjectDef.mesh->GetSurface(surfaceIndex);

            AddDrawSurf(view, nullptr, visibleObject, renderObjectDef.materials[surf->materialIndex], surf->subMesh, flags);

            view->numAmbientSurfs++;

            surf->viewCount = viewCount;
            surf->drawSurf = view->drawSurfs[view->numDrawSurfs - 1];
        }
    }
}

void RenderWorld::AddParticleMeshes(VisibleView *view) {
    for (VisibleObject *visibleObject = view->visibleObjects.Next(); visibleObject; visibleObject = visibleObject->node.Next()) {
        if (!visibleObject->ambientVisible) {
            continue;
        }

        const RenderObject::State &renderObjectDef = visibleObject->def->state;

        if (!renderObjectDef.particleSystem) {
            continue;
        }

        int flags = DrawSurf::AmbientVisible | DrawSurf::SkipSelection;
        if (renderObjectDef.wireframeMode != RenderObject::WireframeMode::ShowNone || r_showWireframe.GetInteger() > 0) {
            flags |= DrawSurf::ShowWires;
        }

        particleMesh.Clear();
        particleMesh.Draw(renderObjectDef.particleSystem, renderObjectDef.stageParticles, visibleObject->def, view->def);
        particleMesh.CacheIndexes();

        for (int surfaceIndex = 0; surfaceIndex < particleMesh.surfaces.Count(); surfaceIndex++) {
            const PrtMeshSurf *prtMeshSurf = &particleMesh.surfaces[surfaceIndex];
            if (!prtMeshSurf->numIndexes) {
                break;
            }

            // Copy this SubMesh to the temporary frame data for use in backend
            SubMesh *subMesh = (SubMesh *)frameData.ClearedAlloc(sizeof(SubMesh));
            new (subMesh) SubMesh();

            subMesh->type           = Mesh::DynamicMesh;
            subMesh->numIndexes     = prtMeshSurf->numIndexes;
            subMesh->numVerts       = prtMeshSurf->numVerts;

            subMesh->vertexCache    = (BufferCache *)frameData.ClearedAlloc(sizeof(BufferCache));
            *(subMesh->vertexCache) = prtMeshSurf->vertexCache;

            subMesh->indexCache     = (BufferCache *)frameData.ClearedAlloc(sizeof(BufferCache));
            *(subMesh->indexCache)  = prtMeshSurf->indexCache;

            AddDrawSurf(view, nullptr, visibleObject, prtMeshSurf->material, subMesh, flags);

            view->numAmbientSurfs++;
        }
    }
}

void RenderWorld::AddTextMeshes(VisibleView *view) {
    for (VisibleObject *visibleObject = view->visibleObjects.Next(); visibleObject; visibleObject = visibleObject->node.Next()) {
        if (!visibleObject->ambientVisible) {
            continue;
        }

        const RenderObject::State &renderObjectDef = visibleObject->def->state;

        if (renderObjectDef.text.IsEmpty()) {
            continue;
        }

        int flags = DrawSurf::AmbientVisible;
        if (renderObjectDef.wireframeMode != RenderObject::WireframeMode::ShowNone || r_showWireframe.GetInteger() > 0) {
            flags |= DrawSurf::ShowWires;
        }

        textMesh.Clear();
        textMesh.SetColor(Color4(&renderObjectDef.materialParms[RenderObject::RedParm]));
        textMesh.Draw(renderObjectDef.font, renderObjectDef.textAnchor, renderObjectDef.textAlignment, renderObjectDef.lineSpacing, renderObjectDef.textScale, renderObjectDef.text);
        textMesh.CacheIndexes();

        for (int surfaceIndex = 0; surfaceIndex < textMesh.NumSurfaces(); surfaceIndex++) {
            const GuiMeshSurf *guiMeshSurf = textMesh.Surface(surfaceIndex);
            if (!guiMeshSurf->numIndexes) {
                break;
            }

            // Copy this SubMesh to the temporary frame data for use in back end
            SubMesh *subMesh = (SubMesh *)frameData.ClearedAlloc(sizeof(SubMesh));
            new (subMesh) SubMesh();

            subMesh->type           = Mesh::DynamicMesh;
            subMesh->numIndexes     = guiMeshSurf->numIndexes;
            subMesh->numVerts       = guiMeshSurf->numVerts;

            subMesh->vertexCache    = (BufferCache *)frameData.ClearedAlloc(sizeof(BufferCache));
            *(subMesh->vertexCache) = guiMeshSurf->vertexCache;

            subMesh->indexCache     = (BufferCache *)frameData.ClearedAlloc(sizeof(BufferCache));
            *(subMesh->indexCache)  = guiMeshSurf->indexCache;

            AddDrawSurf(view, nullptr, visibleObject, guiMeshSurf->material, subMesh, flags);

            view->numAmbientSurfs++;
        }
    }
}

void RenderWorld::AddSkyBoxMeshes(VisibleView *view) {
    if (view->def->state.clearMethod != RenderView::SkyboxClear) {
        return;
    }

    // skybox entity parameters
    RenderObject::State renderObjectDef;
    memset(&renderObjectDef, 0, sizeof(renderObjectDef));
    renderObjectDef.origin = view->def->state.origin;
    renderObjectDef.scale = Vec3::one;
    renderObjectDef.axis = Mat3::identity;
    renderObjectDef.materialParms[RenderObject::TimeScaleParm] = 1.0f;

    static RenderObject renderObject;
    new (&renderObject) RenderObject();
    renderObject.Update(&renderObjectDef);

    // skybox view entity
    VisibleObject *visibleObject = RegisterVisibleObject(view, &renderObject);

    if (view->def->state.orthogonal) {
        Mat4 projMatrix;
        R_SetPerspectiveProjectionMatrix(45, 45, 1, 1000, false, projMatrix);
        visibleObject->modelViewMatrix = view->def->viewMatrix * renderObject.GetObjectToWorldMatrix();
        visibleObject->modelViewProjMatrix = projMatrix * visibleObject->modelViewMatrix;
    } else {
        visibleObject->modelViewMatrix = view->def->viewMatrix * renderObject.GetObjectToWorldMatrix();
        visibleObject->modelViewProjMatrix = view->def->viewProjMatrix * renderObject.GetObjectToWorldMatrix();
    }

    MeshSurf *meshSurf = meshManager.defaultBoxMesh->GetSurface(0);
    AddDrawSurf(view, nullptr, visibleObject, skyboxMaterial, meshSurf->subMesh, DrawSurf::AmbientVisible);

    view->numAmbientSurfs++;
}

// static mesh 들을 visibleLight 의 litSurfs/shadowCasterSurfs 리스트에 담는다.
void RenderWorld::AddStaticMeshesForLights(VisibleView *view) {
    VisibleLight *visibleLight;

    // Called for static mesh surfaces intersecting with each light volume
    // Returns true if it want to proceed next query
    auto addStaticMeshSurfsForLights = [this, view, &visibleLight](int32_t proxyId) -> bool {
        const DbvtProxy *proxy = (const DbvtProxy *)staticMeshDbvt.GetUserData(proxyId);

        MeshSurf *surf = proxy->mesh->GetSurface(proxy->meshSurfIndex);

        if (!surf) {
            return true;
        }

        // Skip first person view only object in sub camera
        if ((proxy->renderObject->state.flags & RenderObject::FirstPersonOnlyFlag) && view->isSubview) {
            return true;
        }

        // Skip 3rd person view only object in sub camera
        if ((proxy->renderObject->state.flags & RenderObject::ThirdPersonOnlyFlag) && !view->isSubview) {
            return true;
        }

        if (!(BIT(visibleLight->def->state.layer) & view->def->state.layerMask)) {
            return true;
        }

        // Skip if the entity is farther than maximum visible distance
        if (proxy->renderObject->state.origin.DistanceSqr(view->def->state.origin) > proxy->renderObject->state.maxVisDist * proxy->renderObject->state.maxVisDist) {
            return true;
        }

        const Material *material = proxy->renderObject->state.materials[surf->materialIndex];

        // Is surface visible for this frame ?
        if (surf->viewCount == this->viewCount) {
            if ((surf->drawSurf->flags & DrawSurf::AmbientVisible) && material->IsLitSurface()) {
                AddLightDrawSurfFromAmbient(view, surf->drawSurf, visibleLight->index, 0);

                visibleLight->litSurfCount++;

                visibleLight->litSurfsAABB.AddAABB(proxy->worldAABB);
            }
        }

        if ((proxy->renderObject->state.flags & RenderObject::CastShadowsFlag) && material->IsShadowCaster()) {
            OBB surfBounds = OBB(surf->subMesh->GetAABB() * proxy->renderObject->state.scale, proxy->renderObject->state.origin, proxy->renderObject->state.axis);
            if (visibleLight->def->CullShadowCasterOBB(surfBounds, view->def->frustum, view->worldAABB)) {
                return true;
            }

            if (surf->viewCount == this->viewCount) {
                AddLightDrawSurfFromAmbient(view, surf->drawSurf, visibleLight->index, 1);
            } else {
                // This surface is not visible but shadow might be visible as a shadow caster.
                // Register a visibleObject used only for shadow caster
                VisibleObject *shadowCasterObject = RegisterVisibleObject(view, proxy->renderObject);
                shadowCasterObject->shadowVisible = true;

                AddDrawSurf(view, visibleLight, shadowCasterObject, material, surf->subMesh, 0);

                surf->viewCount = this->viewCount;
                surf->drawSurf = view->drawSurfs[view->numDrawSurfs - 1];
            }

            visibleLight->shadowCasterSurfCount++;

            visibleLight->shadowCasterAABB.AddAABB(proxy->worldAABB);
        }

        return true;
    };

    for (visibleLight = view->visibleLights.Next(); visibleLight; visibleLight = visibleLight->node.Next()) {
        const RenderLight *renderLight = visibleLight->def;

        switch (renderLight->state.type) {
        case RenderLight::DirectionalLight:
            staticMeshDbvt.Query(renderLight->worldOBB, addStaticMeshSurfsForLights);
            break;
        case RenderLight::PointLight:
            if (renderLight->IsRadiusUniform()) {
                staticMeshDbvt.Query(Sphere(renderLight->GetOrigin(), renderLight->GetRadius()[0]), addStaticMeshSurfsForLights);
            } else {
                staticMeshDbvt.Query(renderLight->worldOBB, addStaticMeshSurfsForLights);
            }
            break;
        case RenderLight::SpotLight:
            staticMeshDbvt.Query(renderLight->worldFrustum, addStaticMeshSurfsForLights);
            break;
        default:
            break;
       }
    }
}

// skinned mesh 들을 visibleLight 의 litSurfs/shadowCasterSurfs 리스트에 담는다.
void RenderWorld::AddSkinnedMeshesForLights(VisibleView *view) {
    VisibleLight *visibleLight;

    // Called for entities intersecting with each light volume
    // Returns true if it want to proceed next query
    auto addShadowCasterObjects = [this, view, &visibleLight](int32_t proxyId) -> bool {
        const DbvtProxy *proxy = (const DbvtProxy *)this->objectDbvt.GetUserData(proxyId);

        // renderObject 가 없다면 mesh 가 아님
        if (!proxy->renderObject) {
            return true;
        }

        if (!proxy->renderObject->state.joints) {
            return true;
        }

        // Skip first person view only object in sub camera 
        if ((proxy->renderObject->state.flags & RenderObject::FirstPersonOnlyFlag) && view->isSubview) {
            return true;
        }

        // Skip 3rd person view only object in sub camera
        if ((proxy->renderObject->state.flags & RenderObject::ThirdPersonOnlyFlag) && !view->isSubview) {
            return true;
        }

        if (!(BIT(visibleLight->def->state.layer) & view->def->state.layerMask)) {
            return true;
        }

        for (int surfaceIndex = 0; surfaceIndex < proxy->renderObject->state.mesh->NumSurfaces(); surfaceIndex++) {
            MeshSurf *surf = proxy->renderObject->state.mesh->GetSurface(surfaceIndex);

            const Material *material = proxy->renderObject->state.materials[surf->materialIndex];

            // 이미 ambient visible surf 로 등록되었고, lighting 이 필요한 surf 라면 litSurfs 리스트에 추가한다.
            if (surf->viewCount == this->viewCount) {
                if ((surf->drawSurf->flags & DrawSurf::AmbientVisible) && material->IsLitSurface()) {
                    AddLightDrawSurfFromAmbient(view, surf->drawSurf, visibleLight->index, 0);

                    visibleLight->litSurfCount++;
                }
            }
        }

        if (proxy->renderObject->visibleObject && proxy->renderObject->visibleObject->ambientVisible) {
            visibleLight->litSurfsAABB.AddAABB(proxy->worldAABB);
        }

        if (!(proxy->renderObject->state.flags & RenderObject::CastShadowsFlag)) {
            return true;
        }

        OBB worldOBB = OBB(proxy->renderObject->GetLocalAABB(), proxy->renderObject->state.origin, proxy->renderObject->state.axis);
        if (visibleLight->def->CullShadowCasterOBB(worldOBB, view->def->frustum, view->worldAABB)) {
            return true;
        }

        VisibleObject *shadowCasterObject = nullptr;
            
        for (int surfaceIndex = 0; surfaceIndex < proxy->renderObject->state.mesh->NumSurfaces(); surfaceIndex++) {
            MeshSurf *surf = proxy->renderObject->state.mesh->GetSurface(surfaceIndex);

            const Material *material = proxy->renderObject->state.materials[surf->materialIndex];

            if (material->IsShadowCaster()) {
                if (surf->viewCount == this->viewCount) {
                    AddLightDrawSurfFromAmbient(view, surf->drawSurf, visibleLight->index, 1);
                } else {
                    // ambient visible 하지 않으므로 shadow 용 visibleObject 를 등록해준다.
                    if (!shadowCasterObject) {
                        shadowCasterObject = RegisterVisibleObject(view, proxy->renderObject);
                        shadowCasterObject->shadowVisible = true;
                    }

                    if (shadowCasterObject->def->state.skeleton && shadowCasterObject->def->state.joints) {
                        shadowCasterObject->def->state.mesh->UpdateSkinningJointCache(shadowCasterObject->def->state.skeleton, shadowCasterObject->def->state.joints);
                    }

                    // Add drawSurf for shadow
                    AddDrawSurf(view, visibleLight, shadowCasterObject, material, surf->subMesh, 0);

                    surf->viewCount = this->viewCount;
                    surf->drawSurf = view->drawSurfs[view->numDrawSurfs - 1];
                }
                
                visibleLight->shadowCasterSurfCount++;
            }
        }

        visibleLight->shadowCasterAABB.AddAABB(proxy->worldAABB);

        return true;
    };
    
    for (visibleLight = view->visibleLights.Next(); visibleLight; visibleLight = visibleLight->node.Next()) {
        const RenderLight *renderLight = visibleLight->def;

        switch (renderLight->state.type) {
        case RenderLight::DirectionalLight:
            objectDbvt.Query(renderLight->worldOBB, addShadowCasterObjects);
            break;
        case RenderLight::PointLight:
            if (renderLight->IsRadiusUniform()) {
                objectDbvt.Query(Sphere(renderLight->GetOrigin(), renderLight->GetRadius()[0]), addShadowCasterObjects);
            } else {
                objectDbvt.Query(renderLight->worldOBB, addShadowCasterObjects);
            }
            break;
        case RenderLight::SpotLight:
            objectDbvt.Query(renderLight->worldFrustum, addShadowCasterObjects);
            break;
        default:
            break;
        }
    }
}

void RenderWorld::CacheInstanceBuffer(VisibleView *view) {
    int numInstances = 0;

    for (VisibleObject *visibleObject = view->visibleObjects.Next(); visibleObject; visibleObject = visibleObject->node.Next()) {
        const RenderObject *renderObject = visibleObject->def;

        if (!renderObject->state.mesh) {
            continue;
        }

        for (int surfaceIndex = 0; surfaceIndex < renderObject->state.mesh->NumSurfaces(); surfaceIndex++) {
            const MeshSurf *surf = renderObject->state.mesh->GetSurface(surfaceIndex);

            if (surf->viewCount != viewCount) {
                continue;
            }

            if (surf->drawSurf->flags & DrawSurf::UseInstancing) {
                InstanceData *instanceData = (InstanceData *)((byte *)renderGlobal.instanceBufferData + numInstances * rhi.HWLimit().uniformBufferOffsetAlignment);

                const Mat3x4 &localToWorldMatrix = renderObject->GetObjectToWorldMatrix();
                instanceData->localToWorldMatrixS = localToWorldMatrix[0];
                instanceData->localToWorldMatrixT = localToWorldMatrix[1];
                instanceData->localToWorldMatrixR = localToWorldMatrix[2];

                /*if (surf->drawSurf->material->GetPass()->shader->GetPropertyInfoHashMap().Get("_PARALLAX")) {
                    Mat3x4 worldToLocalMatrix = Mat3x4(renderObject->state.axis.Transpose(), -renderObject->state.origin);
                    instanceData->worldToLocalMatrixS = worldToLocalMatrix[0];
                    instanceData->worldToLocalMatrixT = worldToLocalMatrix[1];
                    instanceData->worldToLocalMatrixR = worldToLocalMatrix[2];
                }*/

                instanceData->constantColor = surf->drawSurf->material->GetPass()->useOwnerColor ?
                    reinterpret_cast<const Color4 &>(renderObject->state.materialParms[RenderObject::RedParm]) : 
                    surf->drawSurf->material->GetPass()->constantColor;

                visibleObject->instanceIndex = numInstances++;
                break;
            }
        }
    }

    bufferCacheManager.AllocUniform(numInstances * rhi.HWLimit().uniformBufferOffsetAlignment, renderGlobal.instanceBufferData, view->instanceBufferCache);
}

void RenderWorld::OptimizeLights(VisibleView *view) {
    LinkList<VisibleLight> *nextNode;

    // Iterate over light linked list in view
    for (LinkList<VisibleLight> *node = view->visibleLights.NextNode(); node; node = nextNode) {
        nextNode = node->NextNode();

        VisibleLight *visibleLight = node->Owner();

        const AABB lightAABB = visibleLight->def->GetWorldAABB();
        
        // Compute effective AABB
        visibleLight->litSurfsAABB.IntersectSelf(lightAABB);
        visibleLight->shadowCasterAABB.IntersectSelf(lightAABB);
        
        if (visibleLight->def->state.flags & RenderLight::PrimaryLightFlag) {
            view->primaryLight = visibleLight;
            continue;
        }

        Rect screenClipRect;
        if (!view->def->GetClipRectFromAABB(visibleLight->litSurfsAABB, screenClipRect)) {
            node->Remove();
            continue;
        }

        Rect visScissorRect = screenClipRect;
        visScissorRect.y = renderSystem.currentContext->GetRenderingHeight() - (screenClipRect.y + screenClipRect.h);
        visibleLight->scissorRect.IntersectSelf(visScissorRect);

        if (visibleLight->scissorRect.IsEmpty()) {
            node->Remove();
            continue;
        }
    }
}

void RenderWorld::RenderCamera(VisibleView *view) {
    // Find visible lights/objects by querying DBVT using view frustum.
    // And then register each lights/objects to the visible view.
    // renderObject 와 renderLight 의 pointer 에도 연결 (아래 단계에서 다시 한번 dbvt 를 seaching 할때 이미 등록된 visibleLights/visibleObjects 를 한번에 찾기위해)
    FindVisibleLightsAndObjects(view);

    // Add static mesh surface drawSurf by querying staticMeshDBVT
    AddStaticMeshes(view);

    // Add skinned mesh surface drawSurf by searching in visible objects
    AddSkinnedMeshes(view);

    AddParticleMeshes(view);

    AddTextMeshes(view);

    AddSkyBoxMeshes(view);
    
    /*if (!(view->def->state.flags & NoSubViews)) {
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

            RenderSubCamera(drawSurf->entity, drawSurf, drawSurf->material);

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

    if (r_instancing.GetBool()) {
        CacheInstanceBuffer(view);
    }

    // Sort all drawSurfs
    SortDrawSurfs(view);

    OptimizeLights(view);

    renderSystem.CmdDrawView(view);
}

void RenderWorld::RenderSubCamera(VisibleObject *visibleObject, const DrawSurf *drawSurf, const Material *material) {
}

void RenderWorld::AddDrawSurf(VisibleView *view, VisibleLight *visibleLight, VisibleObject *visibleObject, const Material *material, SubMesh *subMesh, int flags) {
    if (view->numDrawSurfs + 1 > view->maxDrawSurfs) {
        BE_WARNLOG(L"RenderWorld::AddDrawSurf: not enough renderable surfaces\n");
        return;
    }

    const Material *realMaterial = material;
    if (!realMaterial) {
        realMaterial = materialManager.defaultMaterial;
    }

    //if (visibleObject->def->state.customSkin) {
    //  realMaterial = (visibleObject->def->state.customSkin)->RemapMaterialBySkin(material);
    //}

    /*float *outputValues = (float *)frameData.Alloc(realMaterial->GetExprChunk()->NumRegisters() * sizeof(float));
    float localParms[MAX_EXPR_LOCALPARMS];
    localParms[0] = view->def->state.time * 0.001f;
    memcpy(&localParms[1], visibleObject->def->state.materialParms, sizeof(visibleObject->def->state.materialParms));

    realMaterial->GetExprChunk()->Evaluate(localParms, outputValues);*/

    if (!bufferCacheManager.IsCached(subMesh->vertexCache)) {
        if (subMesh->GetType() == Mesh::ReferenceMesh ||
            subMesh->GetType() == Mesh::StaticMesh ||
            subMesh->GetType() == Mesh::SkinnedMesh) {
            subMesh->CacheStaticDataToGpu();
        } else {
            subMesh->CacheDynamicDataToGpu(visibleObject->def->state.joints, realMaterial);
        }
    }

    if (r_instancing.GetBool() && realMaterial->GetPass()->instancingEnabled) {
        if (subMesh->IsGpuSkinning()) {
            if (renderGlobal.skinningMethod == Mesh::VertexTextureFetchSkinning) {
                flags |= DrawSurf::UseInstancing;
            }
        } else {
            flags |= DrawSurf::UseInstancing;
        }
    }

    DrawSurf *drawSurf = (DrawSurf *)frameData.ClearedAlloc(sizeof(DrawSurf));

    drawSurf->space = visibleObject;
    drawSurf->material = realMaterial;
    drawSurf->materialRegisters = nullptr;//outputValues;
    drawSurf->subMesh = subMesh;
    drawSurf->flags = flags;

    uint64_t materialSort = realMaterial->GetSort();
    uint64_t objectIndex = visibleObject->index;
    uint64_t materialIndex = materialManager.GetIndexByMaterial(realMaterial);
    uint64_t lightIndex = visibleLight ? visibleLight->index + 1 : 0;
    uint64_t shadowBit = !(flags & DrawSurf::AmbientVisible);

    // Rough sorting back-to-front order for translucent surfaces.
    if (materialSort == Material::TranslucentSort || materialSort == Material::OverlaySort) {
        float depthMin = 0.0f;
        float depthMax = 1.0f;

        view->def->GetDepthBoundsFromAABB(subMesh->GetAABB(), visibleObject->modelViewProjMatrix, &depthMin, &depthMax);

        uint64_t depthDist = Math::Ftoui16(depthMin * 0xFFFF);
        depthDist = 0xFFFF - depthDist;

        //---------------------------------------------------
        // SortKey for translucent materials:       
        // 0x8000000000000000 (0~1)     : shadow bit
        // 0x7FF0000000000000 (0~2047)  : light index
        // 0x000F000000000000 (0~15)    : material sort
        // 0x0000FFFF00000000 (0~65535) : depth dist
        // 0x00000000FFFF0000 (0~65535) : material index
        // 0x000000000000FFFF (0~65535) : object index
        //---------------------------------------------------
        drawSurf->sortKey = ((shadowBit << 63) | (lightIndex << 52) | (materialSort << 48) | (depthDist << 32) | (materialIndex << 16) | objectIndex);
    } else {
        uint64_t subMeshIndex = ((uint64_t)subMesh->subMeshIndex & 0xFFFF);

        //---------------------------------------------------
        // SortKey for opaque materials:
        // 0x8000000000000000 (0~1)     : shadow bit
        // 0x7FF0000000000000 (0~2047)  : light index
        // 0x000F000000000000 (0~15)    : material sort
        // 0x0000FFFF00000000 (0~65535) : sub mesh index
        // 0x00000000FFFF0000 (0~65535) : material index
        // 0x000000000000FFFF (0~65535) : object index
        //---------------------------------------------------
        drawSurf->sortKey = ((shadowBit << 63) | (lightIndex << 52) | (materialSort << 48) | (subMeshIndex << 32) | (materialIndex << 16) | objectIndex);
    }

    view->drawSurfs[view->numDrawSurfs++] = drawSurf;
}

void RenderWorld::AddLightDrawSurfFromAmbient(VisibleView *view, const DrawSurf *from, int lightIndex, int shadowBit) {
    if (view->numDrawSurfs + 1 > view->maxDrawSurfs) {
        BE_WARNLOG(L"RenderWorld::AddLightDrawSurfFromAmbient: not enough renderable surfaces\n");
        return;
    }

    DrawSurf *drawSurf = (DrawSurf *)frameData.Alloc(sizeof(DrawSurf));
    drawSurf->sortKey = (from->sortKey & 0x000FFFFFFFFFFFFF) | ((uint64_t)shadowBit << 63) | ((uint64_t)(lightIndex + 1) << 52);
    drawSurf->flags = from->flags;
    drawSurf->space = from->space;
    drawSurf->material = from->material;
    drawSurf->materialRegisters = from->materialRegisters;
    drawSurf->subMesh = from->subMesh;

    view->drawSurfs[view->numDrawSurfs++] = drawSurf;
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

    int prevLightIndex = -1;
    int prevShadowBit = -1;

    for (int i = 0; i < view->numDrawSurfs; i++) {
        const DrawSurf *drawSurf = view->drawSurfs[i];
        
        int shadowBit  = (drawSurf->sortKey & 0x8000000000000000) >> 63;
        int lightIndex = (drawSurf->sortKey & 0x7FF0000000000000) >> 52;

        if (lightIndex >= 1) {
            if (lightIndex != prevLightIndex || shadowBit != prevShadowBit) {
                if (shadowBit) {
                    renderLights[lightIndex - 1]->visibleLight->shadowCasterSurfFirst = i;
                } else {
                    renderLights[lightIndex - 1]->visibleLight->litSurfFirst = i;
                }

                prevLightIndex = lightIndex;
                prevShadowBit = shadowBit;
            }
        }
    }
}

BE_NAMESPACE_END
