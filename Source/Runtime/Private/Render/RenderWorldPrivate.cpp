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

// Add visible object from render object.
// Prevent to add multiple times in same view.
VisibleObject *RenderWorld::RegisterVisibleObject(VisibleView *visView, RenderObject *renderObject) {
    // Already registered for this frame
    if (renderObject->viewCount == viewCount) {
        return renderObject->visObject;
    }

    VisibleObject *visObject = (VisibleObject *)frameData.ClearedAlloc(sizeof(*visObject));

    new (&visObject->node) LinkList<VisibleObject>();
    visObject->node.SetOwner(visObject);
    visObject->node.AddToEnd(visView->visObjects);

    visObject->index = visView->numVisibleObjects++;

    visObject->def = renderObject;

    // Connect visible object to render object for use in this view frame
    renderObject->visObject = visObject;

    // Increase in RenderCamera()
    renderObject->viewCount = viewCount;

    return visObject;
}

// Add visible light from render light.
// Prevent to add multiple times in same view.
VisibleLight *RenderWorld::RegisterVisibleLight(VisibleView *visView, RenderLight *renderLight) {
    if (renderLight->viewCount == viewCount) {
        // already registered for this frame
        return renderLight->visLight;
    }

    VisibleLight *visLight = (VisibleLight *)frameData.ClearedAlloc(sizeof(*visLight));

    new (&visLight->node) LinkList<VisibleLight>();
    visLight->node.SetOwner(visLight);
    visLight->node.AddToEnd(visView->visLights);

    visLight->index = visView->numVisibleLights++;

    visLight->def = renderLight;

    visLight->litSurfsAABB.Clear();
    visLight->shadowCasterAABB.Clear();

    // Connect visible light to render light for use in this view frame
    renderLight->visLight = visLight;

    // Increase in RenderCamera()
    renderLight->viewCount = viewCount;

    return visLight;
}

// Add visible lights/objects using view volume.
void RenderWorld::FindVisibleLightsAndObjects(VisibleView *visView) {
    viewCount++;

    visView->worldAABB.Clear();
    visView->visLights.Clear();
    visView->visObjects.Clear();

    // Called for each scene lights that intersects with visView frustum.
    // Returns true if it want to proceed next query.
    auto addVisibleLights = [this, visView](int32_t proxyId) -> bool {
        DbvtProxy *proxy = (DbvtProxy *)lightDbvt.GetUserData(proxyId);
        RenderLight *renderLight = proxy->renderLight;

        if (!renderLight) {
            return true;
        }

        // Skip if object layer is not visible with this visView
        if (!(BIT(renderLight->state.layer) & visView->def->state.layerMask)) {
            return true;
        }

        // Skip if a light is farther than maximum visible distance
        if (renderLight->state.origin.DistanceSqr(visView->def->state.origin) > renderLight->state.maxVisDist * renderLight->state.maxVisDist) {
            return true;
        }

        // Cull exact light bounding volume
        if (visView->def->state.orthogonal) {
            if (renderLight->Cull(visView->def->box)) {
                return true;
            }
        } else {
            if (renderLight->Cull(visView->def->frustum)) {
                return true;
            }
        }

        // Calculate light scissor rect
        Rect screenClipRect;
        if (!renderLight->ComputeScreenClipRect(visView->def, screenClipRect)) {
            return true;
        }

        VisibleLight *visLight = RegisterVisibleLight(visView, renderLight);
        visLight->scissorRect = screenClipRect;
        // glScissor 의 x, y 좌표는 lower left corner 이므로 y 좌표를 밑에서 증가되도록 뒤집는다.
        visLight->scissorRect.y = renderSystem.currentContext->GetRenderingHeight() - (screenClipRect.y + screenClipRect.h);

        return true;
    };

    // Called for each scene objects that intersects with visView frustum.
    // Returns true if it want to proceed next query.
    auto addVisibleObjects = [this, visView](int32_t proxyId) -> bool {
        DbvtProxy *proxy = (DbvtProxy *)objectDbvt.GetUserData(proxyId);
        RenderObject *renderObject = proxy->renderObject;

        if (!renderObject) {
            return true;
        }

        // Skip if object layer is not visible with this visView
        if (!(BIT(renderObject->state.layer) & visView->def->state.layerMask)) {
            return true;
        }

        // Skip first person visView only object in sub camera
        if ((renderObject->state.flags & RenderObject::FirstPersonOnlyFlag) && visView->isSubview) {
            return true;
        }

        // Skip 3rd person visView only object in sub camera
        if ((renderObject->state.flags & RenderObject::ThirdPersonOnlyFlag) && !visView->isSubview) {
            return true;
        }

        // Skip if a entity is farther than maximum visible distance
        if (renderObject->state.origin.DistanceSqr(visView->def->state.origin) > renderObject->state.maxVisDist * renderObject->state.maxVisDist) {
            return true;
        }

        // Register visible object form the render object
        VisibleObject *visObject = RegisterVisibleObject(visView, renderObject);

        visObject->ambientVisible = true;
        visObject->modelViewMatrix = visView->def->viewMatrix * renderObject->GetObjectToWorldMatrix();
        visObject->modelViewProjMatrix = visView->def->viewProjMatrix * renderObject->GetObjectToWorldMatrix();

        if (renderObject->state.flags & RenderObject::BillboardFlag) {
            Mat3 inverse = (visView->def->viewMatrix.ToMat3() * renderObject->GetObjectToWorldMatrix().ToMat3()).Inverse();
            //inverse = inverse * Mat3(0, 0, 1, 1, 0, 0, 0, 1, 0);
            Swap(inverse[0], inverse[2]);
            Swap(inverse[1], inverse[2]);

            Mat3 billboardMatrix = inverse * Mat3::FromScale(renderObject->state.scale);
            visObject->modelViewMatrix *= billboardMatrix;
            visObject->modelViewProjMatrix *= billboardMatrix;
        }

        visView->worldAABB.AddAABB(proxy->worldAABB);

        if (r_showAABB.GetInteger() > 0) {
            SetDebugColor(Color4::blue, Color4::zero);
            DebugAABB(proxy->worldAABB, 1, true, r_showAABB.GetInteger() == 1 ? true : false);
        }

        if (visObject->def->state.numJoints > 0 && r_showSkeleton.GetInteger() > 0) {
            DebugJoints(visObject->def, r_showSkeleton.GetInteger() == 2, visView->def->state.axis);
        }

        return true;
    };

    if (visView->def->state.orthogonal) {
        lightDbvt.Query(visView->def->box, addVisibleLights);
        objectDbvt.Query(visView->def->box, addVisibleObjects);
    } else {
        lightDbvt.Query(visView->def->frustum, addVisibleLights);
        objectDbvt.Query(visView->def->frustum, addVisibleObjects);
    }
}

// Add drawSurf for visible static meshes.
void RenderWorld::AddStaticMeshes(VisibleView *visView) {
    // Called for each static mesh surfaces intersecting with visView frustum 
    // Returns true if it want to proceed next query
    auto addStaticMeshSurfs = [this, visView](int32_t proxyId) -> bool {
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
            float area = visView->def->state.axis[0].Abs().Dot(Vec3(size.y * size.z, size.z * size.x, size.x * size.y));
            float d = visView->def->state.origin.DistanceSqr(proxy->renderObject->state.origin);
            float lodValue = area / d;
        }*/

#if 0
        // More accurate OBB culling
        OBB obb = OBB(proxy->renderObject->GetAABB(), proxy->renderObject->state.origin, proxy->renderObject->state.axis);
        if (visView->def->frustum.CullOBB(obb)) {
            return true;
        }
#endif

        int flags = DrawSurf::AmbientVisible;
        if (proxy->renderObject->state.wireframeMode != RenderObject::WireframeMode::ShowNone || r_showWireframe.GetInteger() > 0) {
            flags |= DrawSurf::ShowWires;
        }

        VisibleObject *visObject = proxy->renderObject->visObject;
        AddDrawSurf(visView, nullptr, visObject, visObject->def->state.materials[surf->materialIndex], surf->subMesh, flags);

        visView->numAmbientSurfs++;

        surf->viewCount = this->viewCount;
        surf->drawSurf = visView->drawSurfs[visView->numDrawSurfs - 1];

        if (r_showAABB.GetInteger() > 0) {
            SetDebugColor(Color4(1, 1, 1, 0.5), Color4::zero);
            DebugAABB(proxy->worldAABB, 1, true, r_showAABB.GetInteger() == 1 ? true : false);
        }

        return true;
    };

    if (visView->def->state.orthogonal) {
        staticMeshDbvt.Query(visView->def->box, addStaticMeshSurfs);
    } else {
        staticMeshDbvt.Query(visView->def->frustum, addStaticMeshSurfs);
    }
}

// Add drawSurf for visible skinned meshes.
void RenderWorld::AddSkinnedMeshes(VisibleView *visView) {
    for (VisibleObject *visObject = visView->visObjects.Next(); visObject; visObject = visObject->node.Next()) {
        if (!visObject->ambientVisible) {
            continue;
        }

        const RenderObject::State &renderObjectDef = visObject->def->state;

        if (!renderObjectDef.mesh || !renderObjectDef.joints) {
            continue;
        }

#if 0
        // More accurate OBB culling
        OBB obb = OBB(visObject->def->GetAABB(), renderObjectDef.origin, renderObjectDef.axis);
        if (visView->def->frustum.CullOBB(obb)) {
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

            AddDrawSurf(visView, nullptr, visObject, renderObjectDef.materials[surf->materialIndex], surf->subMesh, flags);

            visView->numAmbientSurfs++;

            surf->viewCount = viewCount;
            surf->drawSurf = visView->drawSurfs[visView->numDrawSurfs - 1];
        }
    }
}

// Add drawSurf for visible particle meshes.
void RenderWorld::AddParticleMeshes(VisibleView *visView) {
    for (VisibleObject *visObject = visView->visObjects.Next(); visObject; visObject = visObject->node.Next()) {
        if (!visObject->ambientVisible) {
            continue;
        }

        const RenderObject::State &renderObjectDef = visObject->def->state;

        if (!renderObjectDef.particleSystem) {
            continue;
        }

        int flags = DrawSurf::AmbientVisible | DrawSurf::SkipSelection;
        if (renderObjectDef.wireframeMode != RenderObject::WireframeMode::ShowNone || r_showWireframe.GetInteger() > 0) {
            flags |= DrawSurf::ShowWires;
        }

        particleMesh.Clear();
        particleMesh.Draw(renderObjectDef.particleSystem, renderObjectDef.stageParticles, visObject->def, visView->def);
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

            AddDrawSurf(visView, nullptr, visObject, prtMeshSurf->material, subMesh, flags);

            visView->numAmbientSurfs++;
        }
    }
}

// Add drawSurf for visible text meshes.
void RenderWorld::AddTextMeshes(VisibleView *visView) {
    for (VisibleObject *visObject = visView->visObjects.Next(); visObject; visObject = visObject->node.Next()) {
        if (!visObject->ambientVisible) {
            continue;
        }

        const RenderObject::State &renderObjectDef = visObject->def->state;

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

            AddDrawSurf(visView, nullptr, visObject, guiMeshSurf->material, subMesh, flags);

            visView->numAmbientSurfs++;
        }
    }
}

// Add drawSurf for an skybox.
void RenderWorld::AddSkyBoxMeshes(VisibleView *visView) {
    if (visView->def->state.clearMethod != RenderView::SkyboxClear) {
        return;
    }

    // Skybox object parameters
    RenderObject::State renderObjectDef;
    memset(&renderObjectDef, 0, sizeof(renderObjectDef));
    renderObjectDef.origin = visView->def->state.origin;
    renderObjectDef.scale = Vec3(visView->def->zNear * 4);
    renderObjectDef.axis = Mat3::identity;
    renderObjectDef.materialParms[RenderObject::TimeScaleParm] = 1.0f;

    static RenderObject renderObject;
    new (&renderObject) RenderObject();
    renderObject.Update(&renderObjectDef);

    // Add skybox object
    VisibleObject *visObject = RegisterVisibleObject(visView, &renderObject);

    if (visView->def->state.orthogonal) {
        Mat4 projMatrix;
        R_SetPerspectiveProjectionMatrix(45, 45, 0.01, 1000, false, projMatrix);
        visObject->modelViewMatrix = visView->def->viewMatrix * renderObject.GetObjectToWorldMatrix();
        visObject->modelViewProjMatrix = projMatrix * visObject->modelViewMatrix;
    } else {
        visObject->modelViewMatrix = visView->def->viewMatrix * renderObject.GetObjectToWorldMatrix();
        visObject->modelViewProjMatrix = visView->def->viewProjMatrix * renderObject.GetObjectToWorldMatrix();
    }

    MeshSurf *meshSurf = meshManager.defaultBoxMesh->GetSurface(0);
    AddDrawSurf(visView, nullptr, visObject, skyboxMaterial, meshSurf->subMesh, DrawSurf::AmbientVisible);

    visView->numAmbientSurfs++;
}

// Add lit drawSurfs for visible static meshes for each light.
void RenderWorld::AddStaticMeshesForLights(VisibleView *visView) {
    VisibleLight *visLight;

    // Called for static mesh surfaces intersecting with each light volume
    // Returns true if it want to proceed next query
    auto addStaticMeshSurfsForLights = [this, visView, &visLight](int32_t proxyId) -> bool {
        const DbvtProxy *proxy = (const DbvtProxy *)staticMeshDbvt.GetUserData(proxyId);

        RenderObject *renderObject = proxy->renderObject;

        MeshSurf *surf = proxy->mesh->GetSurface(proxy->meshSurfIndex);

        if (!surf) {
            return true;
        }

        // Skip first person visView only object in sub camera
        if ((renderObject->state.flags & RenderObject::FirstPersonOnlyFlag) && visView->isSubview) {
            return true;
        }

        // Skip 3rd person visView only object in sub camera
        if ((renderObject->state.flags & RenderObject::ThirdPersonOnlyFlag) && !visView->isSubview) {
            return true;
        }

        // Skip if the entity is farther than maximum visible distance
        if (renderObject->state.origin.DistanceSqr(visView->def->state.origin) > renderObject->state.maxVisDist * renderObject->state.maxVisDist) {
            return true;
        }

        const Material *material = renderObject->state.materials[surf->materialIndex];
        bool isShadowCaster = false;

        if (visLight->def->state.flags & RenderLight::CastShadowsFlag) {
            if ((renderObject->state.flags & RenderObject::CastShadowsFlag) && material->IsShadowCaster()) {
                OBB surfBounds = OBB(surf->subMesh->GetAABB() * renderObject->state.scale, renderObject->state.origin, renderObject->state.axis);
                if (!visLight->def->CullShadowCasterOBB(surfBounds, visView->def->frustum, visView->worldAABB)) {
                    isShadowCaster = true;
                }
            }
        }

        // Already visible in this frame
        if (surf->viewCount == this->viewCount) {
            if ((surf->drawSurf->flags & DrawSurf::AmbientVisible) && material->IsLitSurface()) {
                // Add drawSurf from visible drawSurf
                AddDrawSurfFromAmbient(visView, visLight, isShadowCaster, surf->drawSurf);

                visLight->numDrawSurfs++;
                visLight->litSurfsAABB.AddAABB(proxy->worldAABB);

                if (isShadowCaster) {
                    visLight->shadowCasterAABB.AddAABB(proxy->worldAABB);
                }
            }
        } else if (isShadowCaster) {
            // This surface is not visible but shadow might be visible as a shadow caster.
            // Register a visObject used only for shadow caster
            VisibleObject *shadowCasterObject = RegisterVisibleObject(visView, renderObject);
            shadowCasterObject->shadowVisible = true;

            AddDrawSurf(visView, visLight, shadowCasterObject, material, surf->subMesh, DrawSurf::ShadowCaster);

            surf->viewCount = this->viewCount;
            surf->drawSurf = visView->drawSurfs[visView->numDrawSurfs - 1];

            visLight->numDrawSurfs++;
            visLight->shadowCasterAABB.AddAABB(proxy->worldAABB);
        }

        return true;
    };

    for (visLight = visView->visLights.Next(); visLight; visLight = visLight->node.Next()) {
        const RenderLight *renderLight = visLight->def;

        if (!(BIT(visLight->def->state.layer) & visView->def->state.layerMask)) {
            continue;
        }

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

// Add lit drawSurfs for visible skinned meshes for each light.
void RenderWorld::AddSkinnedMeshesForLights(VisibleView *visView) {
    VisibleLight *visLight;

    // Called for entities intersecting with each light volume
    // Returns true if it want to proceed next query
    auto addShadowCasterObjects = [this, visView, &visLight](int32_t proxyId) -> bool {
        const DbvtProxy *proxy = (const DbvtProxy *)this->objectDbvt.GetUserData(proxyId);
        RenderObject *renderObject = proxy->renderObject;

        // renderObject 가 없다면 mesh 가 아님
        if (!renderObject) {
            return true;
        }

        if (!renderObject->state.joints) {
            return true;
        }

        // Skip first person visView only object in sub camera 
        if ((renderObject->state.flags & RenderObject::FirstPersonOnlyFlag) && visView->isSubview) {
            return true;
        }

        // Skip 3rd person visView only object in sub camera
        if ((renderObject->state.flags & RenderObject::ThirdPersonOnlyFlag) && !visView->isSubview) {
            return true;
        }

        bool isShadowCaster = false;

        if ((visLight->def->state.flags & RenderLight::CastShadowsFlag) && (renderObject->state.flags & RenderObject::CastShadowsFlag)) {
            OBB worldOBB = OBB(renderObject->GetLocalAABB(), renderObject->state.origin, renderObject->state.axis);
            if (!visLight->def->CullShadowCasterOBB(worldOBB, visView->def->frustum, visView->worldAABB)) {
                isShadowCaster = true;
            }
        }

        VisibleObject *shadowCasterObject = nullptr;

        for (int surfaceIndex = 0; surfaceIndex < renderObject->state.mesh->NumSurfaces(); surfaceIndex++) {
            MeshSurf *surf = renderObject->state.mesh->GetSurface(surfaceIndex);

            const Material *material = renderObject->state.materials[surf->materialIndex];

            // Already visible in this frame
            if (surf->viewCount == this->viewCount) {
                if ((surf->drawSurf->flags & DrawSurf::AmbientVisible) && material->IsLitSurface()) {
                    // Add drawSurf from visible drawSurf
                    AddDrawSurfFromAmbient(visView, visLight, isShadowCaster && material->IsShadowCaster(), surf->drawSurf);

                    visLight->numDrawSurfs++;
                    visLight->litSurfsAABB.AddAABB(proxy->worldAABB);

                    if (isShadowCaster && material->IsShadowCaster()) {
                        visLight->shadowCasterAABB.AddAABB(proxy->worldAABB);
                    }
                }
            } else if (isShadowCaster && material->IsShadowCaster()) {
                // This surface is not visible but shadow might be visible as a shadow caster.
                // Register a visObject used only for shadow caster
                if (!shadowCasterObject) {
                    shadowCasterObject = RegisterVisibleObject(visView, renderObject);
                    shadowCasterObject->shadowVisible = true;
                }

                if (shadowCasterObject->def->state.skeleton && shadowCasterObject->def->state.joints) {
                    shadowCasterObject->def->state.mesh->UpdateSkinningJointCache(shadowCasterObject->def->state.skeleton, shadowCasterObject->def->state.joints);
                }

                AddDrawSurf(visView, visLight, shadowCasterObject, material, surf->subMesh, DrawSurf::ShadowCaster);

                surf->viewCount = this->viewCount;
                surf->drawSurf = visView->drawSurfs[visView->numDrawSurfs - 1];

                visLight->numDrawSurfs++;
                visLight->shadowCasterAABB.AddAABB(proxy->worldAABB);
            }
        }

        return true;
    };
    
    for (visLight = visView->visLights.Next(); visLight; visLight = visLight->node.Next()) {
        const RenderLight *renderLight = visLight->def;

        if (!(BIT(visLight->def->state.layer) & visView->def->state.layerMask)) {
            continue;
        }

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

void RenderWorld::CacheInstanceBuffer(VisibleView *visView) {
    int numInstances = 0;

    for (VisibleObject *visObject = visView->visObjects.Next(); visObject; visObject = visObject->node.Next()) {
        const RenderObject *renderObject = visObject->def;

        if (!renderObject->state.mesh) {
            continue;
        }

        for (int surfaceIndex = 0; surfaceIndex < renderObject->state.mesh->NumSurfaces(); surfaceIndex++) {
            const MeshSurf *surf = renderObject->state.mesh->GetSurface(surfaceIndex);

            if (surf->viewCount != viewCount) {
                continue;
            }

            if (surf->drawSurf->flags & DrawSurf::UseInstancing) {
                byte *instanceData = ((byte *)renderGlobal.instanceBufferData + numInstances * renderGlobal.instanceBufferOffsetAlignment);

                const Mat3x4 &localToWorldMatrix = renderObject->GetObjectToWorldMatrix();
                *(Mat3x4 *)instanceData = localToWorldMatrix; 
                instanceData += 48;

                /*if (surf->drawSurf->material->GetPass()->shader->GetPropertyInfoHashMap().Get("_PARALLAX")) {
                    Mat3x4 worldToLocalMatrix = Mat3x4(renderObject->state.axis.Transpose(), -renderObject->state.origin);
                    *(Mat3x4 *)instanceData = worldToLocalMatrix; 
                    instanceData += 48;
                }*/

                if (renderGlobal.instancingMethod == Mesh::InstancedArraysInstancing) {
                    if (surf->drawSurf->material->GetPass()->useOwnerColor) {
                        *(uint32_t *)instanceData = Color4(&renderObject->state.materialParms[RenderObject::RedParm]).ToUInt32();
                    } else {
                        *(uint32_t *)instanceData = surf->drawSurf->material->GetPass()->constantColor.ToUInt32();
                    }
                    instanceData += 4;
                } else {
                    if (surf->drawSurf->material->GetPass()->useOwnerColor) {
                        *(Color4 *)instanceData = Color4(&renderObject->state.materialParms[RenderObject::RedParm]);
                    } else {
                        *(Color4 *)instanceData = surf->drawSurf->material->GetPass()->constantColor;
                    }
                    instanceData += 16;
                }

                if (surf->subMesh->IsGpuSkinning()) {
                    const SkinningJointCache *skinningJointCache = renderObject->state.mesh->skinningJointCache;

                    if (renderGlobal.vtUpdateMethod == BufferCacheManager::TboUpdate) {
                        *(uint32_t *)instanceData = (uint32_t)skinningJointCache->GetBufferCache().tcBase[0];
                    } else {
                        *(Vec2 *)instanceData = Vec2(skinningJointCache->GetBufferCache().tcBase[0], skinningJointCache->GetBufferCache().tcBase[1]);
                    }
                }

                visObject->instanceIndex = numInstances++;
                break;
            }
        }
    }

    if (numInstances > 0) {
        if (renderGlobal.instancingMethod == Mesh::InstancedArraysInstancing) {
            bufferCacheManager.AllocVertex(numInstances, renderGlobal.instanceBufferOffsetAlignment, renderGlobal.instanceBufferData, visView->instanceBufferCache);
        } else if (renderGlobal.instancingMethod == Mesh::UniformBufferInstancing) {
            bufferCacheManager.AllocUniform(numInstances * renderGlobal.instanceBufferOffsetAlignment, renderGlobal.instanceBufferData, visView->instanceBufferCache);
        }
    }
}

void RenderWorld::OptimizeLights(VisibleView *visView) {
    LinkList<VisibleLight> *nextNode;

    // Iterate over light linked list in visView
    for (LinkList<VisibleLight> *node = visView->visLights.NextNode(); node; node = nextNode) {
        nextNode = node->NextNode();

        VisibleLight *visLight = node->Owner();

        if (visLight->numDrawSurfs == 0) {
            node->Remove();

            visView->numVisibleLights--;
            continue;
        }

        const AABB lightAABB = visLight->def->GetWorldAABB();
        
        // Compute effective AABB
        visLight->litSurfsAABB.IntersectSelf(lightAABB);
        visLight->shadowCasterAABB.IntersectSelf(lightAABB);
        
        if (visLight->def->state.flags & RenderLight::PrimaryLightFlag) {
            visView->primaryLight = visLight;
            continue;
        }

        Rect screenClipRect;
        if (!visView->def->GetClipRectFromAABB(visLight->litSurfsAABB, screenClipRect)) {
            node->Remove();

            visView->numVisibleLights--;
            continue;
        }

        Rect visScissorRect = screenClipRect;
        visScissorRect.y = renderSystem.currentContext->GetRenderingHeight() - (screenClipRect.y + screenClipRect.h);
        visLight->scissorRect.IntersectSelf(visScissorRect);

        if (visLight->scissorRect.IsEmpty()) {
            node->Remove();

            visView->numVisibleLights--;
            continue;
        }
    }
}

void RenderWorld::RenderCamera(VisibleView *visView) {
    // Find visible lights/objects by querying DBVT using view frustum.
    // And then register each lights/objects to the visible view.
    // 이번 프레임을 위해 renderObject 와 renderLight 의 pointer 에도 연결 (아래 단계에서 다시 한번 dbvt 를 seaching 할때 이미 등록된 visLights/visLights 를 한번에 찾기위해)
    FindVisibleLightsAndObjects(visView);

    // Add static mesh surface drawSurf by querying staticMeshDBVT
    AddStaticMeshes(visView);

    // Add skinned mesh surface drawSurf by searching in visible objects
    AddSkinnedMeshes(visView);

    AddParticleMeshes(visView);

    AddTextMeshes(visView);

    AddSkyBoxMeshes(visView);
    
    /*if (!(visView->def->state.flags & NoSubViews)) {
        for (int i = 0; i < visView->numDrawSurfs; i++) {
            DrawSurf *drawSurf = visView->drawSurfs[i];

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
    
    AddStaticMeshesForLights(visView);

    AddSkinnedMeshesForLights(visView);

    OptimizeLights(visView);

    if (renderGlobal.instancingMethod != Mesh::NoInstancing) {
        CacheInstanceBuffer(visView);
    }

    SortDrawSurfs(visView);

    renderSystem.CmdDrawView(visView);
}

void RenderWorld::RenderSubCamera(VisibleObject *visObject, const DrawSurf *drawSurf, const Material *material) {
}

void RenderWorld::AddDrawSurf(VisibleView *visView, VisibleLight *visLight, VisibleObject *visObject, const Material *material, SubMesh *subMesh, int flags) {
    if (visView->numDrawSurfs + 1 > visView->maxDrawSurfs) {
        BE_WARNLOG(L"RenderWorld::AddDrawSurf: not enough renderable surfaces\n");
        return;
    }

    const Material *actualMaterial = material;
    if (!actualMaterial) {
        actualMaterial = materialManager.defaultMaterial;
    }

    //if (visObject->def->state.customSkin) {
    //  actualMaterial = (visObject->def->state.customSkin)->RemapMaterialBySkin(material);
    //}

    /*float *outputValues = (float *)frameData.Alloc(actualMaterial->GetExprChunk()->NumRegisters() * sizeof(float));
    float localParms[MAX_EXPR_LOCALPARMS];
    localParms[0] = visView->def->state.time * 0.001f;
    memcpy(&localParms[1], visObject->def->state.materialParms, sizeof(visObject->def->state.materialParms));

    actualMaterial->GetExprChunk()->Evaluate(localParms, outputValues);*/

    if (!bufferCacheManager.IsCached(subMesh->vertexCache)) {
        if (subMesh->GetType() == Mesh::ReferenceMesh ||
            subMesh->GetType() == Mesh::StaticMesh ||
            subMesh->GetType() == Mesh::SkinnedMesh) {
            subMesh->CacheStaticDataToGpu();
        } else {
            subMesh->CacheDynamicDataToGpu(visObject->def->state.joints, actualMaterial);
        }
    }

    if (renderGlobal.instancingMethod != Mesh::NoInstancing) {
        if (actualMaterial->GetPass()->instancingEnabled) {
            if (subMesh->IsGpuSkinning()) {
                if (renderGlobal.skinningMethod == SkinningJointCache::VertexTextureFetchSkinning) {
                    flags |= DrawSurf::UseInstancing;
                }
            } else {
                flags |= DrawSurf::UseInstancing;
            }
        }
    }

    DrawSurf *drawSurf = (DrawSurf *)frameData.ClearedAlloc(sizeof(DrawSurf));

    drawSurf->space = visObject;
    drawSurf->material = actualMaterial;
    drawSurf->materialRegisters = nullptr;//outputValues;
    drawSurf->subMesh = subMesh;
    drawSurf->flags = flags;

    uint64_t visLightIndex = visLight ? visLight->index + 1 : 0;
    uint64_t visObjectIndex = visObject->index;
    uint64_t materialSort = actualMaterial->GetSort();
    uint64_t materialIndex = materialManager.GetIndexByMaterial(actualMaterial);

    // Rough sorting back-to-front order for translucent surfaces.
    if (materialSort == Material::TranslucentSort || materialSort == Material::OverlaySort) {
        float depthMin = 0.0f;
        float depthMax = 1.0f;

        visView->def->GetDepthBoundsFromAABB(subMesh->GetAABB(), visObject->modelViewProjMatrix, &depthMin, &depthMax);

        uint64_t depthDist = Math::Ftoui16(depthMin * 0xFFFF);
        depthDist = 0xFFFF - depthDist;

        //---------------------------------------------------
        // SortKey for translucent materials:
        // 0xFFF0000000000000 (0~4095)  : visible light index
        // 0x000F000000000000 (0~15)    : material sort
        // 0x0000FFFF00000000 (0~65535) : depth distance
        // 0x00000000FFFF0000 (0~65535) : material index
        // 0x000000000000FFFF (0~65535) : visible object index
        //---------------------------------------------------
        drawSurf->sortKey = ((visLightIndex << 52) | (materialSort << 48) | (depthDist << 32) | (materialIndex << 16) | visObjectIndex);
    } else {
        uint64_t subMeshIndex = ((uint64_t)subMesh->subMeshIndex & 0xFFFF);

        //---------------------------------------------------
        // SortKey for opaque materials:
        // 0xFFF0000000000000 (0~4095)  : visible light index
        // 0x000F000000000000 (0~15)    : material sort
        // 0x0000FFFF00000000 (0~65535) : sub mesh index
        // 0x00000000FFFF0000 (0~65535) : material index
        // 0x000000000000FFFF (0~65535) : visible object index
        //---------------------------------------------------
        drawSurf->sortKey = ((visLightIndex << 52) | (materialSort << 48) | (subMeshIndex << 32) | (materialIndex << 16) | visObjectIndex);
    }

    visView->drawSurfs[visView->numDrawSurfs++] = drawSurf;
}

void RenderWorld::AddDrawSurfFromAmbient(VisibleView *visView, const VisibleLight *visLight, bool isShadowCaster, const DrawSurf *ambientDrawSurf) {
    if (visView->numDrawSurfs + 1 > visView->maxDrawSurfs) {
        BE_WARNLOG(L"RenderWorld::AddDrawSurfFromAmbient: not enough renderable surfaces\n");
        return;
    }

    DrawSurf *drawSurf = (DrawSurf *)frameData.Alloc(sizeof(DrawSurf));
    drawSurf->sortKey = (ambientDrawSurf->sortKey & 0x000FFFFFFFFFFFFF) | ((uint64_t)(visLight->index + 1) << 52);
    drawSurf->flags = ambientDrawSurf->flags | (isShadowCaster ? DrawSurf::ShadowCaster : 0);
    drawSurf->space = ambientDrawSurf->space;
    drawSurf->material = ambientDrawSurf->material;
    drawSurf->materialRegisters = ambientDrawSurf->materialRegisters;
    drawSurf->subMesh = ambientDrawSurf->subMesh;

    visView->drawSurfs[visView->numDrawSurfs++] = drawSurf;
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

void RenderWorld::SortDrawSurfs(VisibleView *visView) {
    qsort(visView->drawSurfs, visView->numDrawSurfs, sizeof(DrawSurf *), _CompareDrawSurf);

    VisibleLight *visLight = visView->visLights.Next();

    for (int i = 0; i < visView->numDrawSurfs; i++) {
        if (!visLight) {
            break;
        }
        
        int lightIndex = (visView->drawSurfs[i]->sortKey & 0xFFF0000000000000) >> 52;

        if (lightIndex - 1 == visLight->index) {
            visLight->firstDrawSurf = i;
            assert(i + visLight->numDrawSurfs <= visView->numDrawSurfs);

            visLight = visLight->node.Next();
        }
    }
}

BE_NAMESPACE_END
