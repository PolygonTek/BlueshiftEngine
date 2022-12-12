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
#include "SIMD/SIMD.h"
#include "Profiler/Profiler.h"

BE_NAMESPACE_BEGIN

// Add visObject from renderObject.
// Prevent to add multiple times in same visCamera.
VisObject *RenderWorld::RegisterVisObject(VisCamera *camera, RenderObject *renderObject) {
    // Already registered for this frame
    if (renderObject->viewCount == viewCount) {
        return renderObject->visObject;
    }

    VisObject *visObject = (VisObject *)frameData.ClearedAlloc(sizeof(*visObject));

    new (&visObject->node) LinkList<VisObject>();
    visObject->node.SetOwner(visObject);
    visObject->node.AddToEnd(camera->visObjects);

    visObject->index = camera->numVisibleObjects++;

    visObject->def = renderObject; 

    // Connect visObject to renderObject for use in this frame next time.
    renderObject->visObject = visObject;

    // Calling DrawCamera() increase viewCount.
    renderObject->viewCount = viewCount;

    return visObject;
}

// Add visLight from renderLight.
// Prevent to add multiple times in visCamera.
VisLight *RenderWorld::RegisterVisLight(VisCamera *camera, RenderLight *renderLight) {
    if (renderLight->viewCount == viewCount) {
        // already registered for this frame
        return renderLight->visLight;
    }

    VisLight *visLight = (VisLight *)frameData.ClearedAlloc(sizeof(*visLight));

    new (&visLight->node) LinkList<VisLight>();
    visLight->node.SetOwner(visLight);
    visLight->node.AddToEnd(camera->visLights);

    visLight->index = camera->numVisibleLights++;
    visLight->def = renderLight;

    visLight->litSurfsAABB.Clear();
    visLight->shadowCastersAABB.Clear();

    // Connect visLight to renderLight for use in this frame next time.
    renderLight->visLight = visLight;

    // Calling DrawCamera() increase viewCount.
    renderLight->viewCount = viewCount;

    return visLight;
}

// Add visible lights/objects using bounding view volume.
void RenderWorld::FindVisLightsAndObjects(VisCamera *camera) {
    BE_PROFILE_CPU_SCOPE_STATIC("RenderWorld::FindVisLightsAndObjects");

    camera->worldAABB.Clear();
    camera->visLights.Clear();
    camera->visObjects.Clear();

    // Called for each scene lights that intersects with camera frustum.
    // Returns true if it want to proceed next query.
    auto addVisibleLights = [this, camera](int32_t proxyId) -> bool {
        const DbvtProxy *proxy = (const DbvtProxy *)lightDbvt.GetUserData(proxyId);
        RenderLight *renderLight = proxy->renderLight;

        if (!renderLight) {
            return true;
        }

        // Skip if light layer is not visible with this camera
        if (!(BIT(renderLight->state.layer) & camera->def->GetState().layerMask)) {
            return true;
        }

        // Skip if camera uses static lights and light is not static
        if (camera->def->GetState().flags & RenderCamera::Flag::StaticOnly) {
            if (!(renderLight->state.staticMask & camera->def->GetState().staticMask)) {
                return true;
            }
        }

        // Skip if a light is farther than maximum visible distance
        if (renderLight->state.origin.DistanceSqr(camera->def->GetState().origin) > renderLight->maxVisDistSquared) {
            return true;
        }

        // Cull exact light bounding volume
        if (renderLight->Cull(*camera->def)) {
            return true;
        }

        // Calculate light scissor rect
        Rect screenClipRect;
        if (!renderLight->ComputeScreenClipRect(camera->def, screenClipRect)) {
            return true;
        }

        VisLight *visLight = RegisterVisLight(camera, renderLight);
        visLight->scissorRect = screenClipRect;
        // Since the x and y coordinates of glScissor are lower left corner, we flip the y coordinates to increase from the bottom.
        visLight->scissorRect.y = camera->def->state.renderRect.h - (screenClipRect.y + screenClipRect.h);

        return true;
    };

    // Called for each scene objects that intersects with camera frustum.
    // Returns true if it want to proceed next query.
    auto addVisibleObjects = [this, camera](int32_t proxyId) -> bool {
        const DbvtProxy *proxy = (const DbvtProxy *)objectDbvt.GetUserData(proxyId);
        RenderObject *renderObject = proxy->renderObject;

        if (!renderObject) {
            return true;
        }

        if (renderObject->state.flags & RenderObject::Flag::SkipRendering) {
            return true;
        }

        // Skip if object layer is not visible with this camera.
        if (!(BIT(renderObject->state.layer) & camera->def->GetState().layerMask)) {
            return true;
        }

        // Skip if camera renders static objects and this object is not static.
        if (camera->def->GetState().flags & RenderCamera::Flag::StaticOnly) {
            if (!(renderObject->state.staticMask & camera->def->GetState().staticMask)) {
                return true;
            }
        }

        // Skip first person camera only object in sub camera.
        if ((renderObject->state.flags & RenderObject::Flag::FirstPersonOnly) && camera->isSubCamera) {
            return true;
        }

        // Skip 3rd person camera only object in sub camera.
        if ((renderObject->state.flags & RenderObject::Flag::ThirdPersonOnly) && !camera->isSubCamera) {
            return true;
        }

        // Skip if a object is farther than maximum visible distance.
        if (!(renderObject->state.flags & RenderObject::Flag::NoVisDist)) {
            if (renderObject->state.worldMatrix.ToTranslationVec3().DistanceSqr(camera->def->GetState().origin) > renderObject->maxVisDistSquared) {
                return true;
            }
        }

        // Register visible object form the render object.
        VisObject *visObject = RegisterVisObject(camera, renderObject);

        visObject->ambientVisible = true;
        visObject->modelViewMatrix = camera->def->viewMatrix * renderObject->GetWorldMatrix();
        visObject->modelViewProjMatrix = camera->def->viewProjMatrix * renderObject->GetWorldMatrix();

        if (renderObject->state.flags & RenderObject::Flag::Billboard) {
            Mat3 inverse = (camera->def->viewMatrix.ToMat3() * renderObject->GetWorldMatrix().ToMat3()).Inverse();
            //inverse = inverse * Mat3(0, 0, 1, 1, 0, 0, 0, 1, 0);
            Swap(inverse[0], inverse[2]);
            Swap(inverse[1], inverse[2]);

            Mat3 billboardMatrix = inverse * Mat3::FromScale(renderObject->GetWorldMatrix().ToScaleVec3());
            visObject->modelViewMatrix *= billboardMatrix;
            visObject->modelViewProjMatrix *= billboardMatrix;
        }

        camera->worldAABB.AddAABB(proxy->worldAABB);

        if (renderObject->state.flags & RenderObject::Flag::EnvProbeLit) {
            Array<EnvProbeBlendInfo> localEnvProbes;
            GetClosestProbes(proxy->worldAABB, r_probeBlending.GetBool() ? EnvProbeBlending::Blending : EnvProbeBlending::Simple, localEnvProbes);

            if (localEnvProbes.Count() > 0) {
                visObject->envProbeInfo[0] = localEnvProbes[0];
            } else {
                visObject->envProbeInfo[0].envProbe = distantEnvProbe;
                visObject->envProbeInfo[0].weight = 1.0f;
            }

            if (localEnvProbes.Count() > 1 && localEnvProbes[1].weight > 0.0f) {
                visObject->envProbeInfo[1] = localEnvProbes[1];
            } else {
                visObject->envProbeInfo[1].envProbe = nullptr;
                visObject->envProbeInfo[1].weight = 0.0f;
            }
        } else {
            visObject->envProbeInfo[0].envProbe = nullptr;
            visObject->envProbeInfo[1].envProbe = nullptr;
        }

        if (r_showAABB.GetInteger() > 0) {
            SetDebugColor(Color4(0.0f, 0.0f, 1.0f, 1.0f), Color4::zero);
            DebugAABB(proxy->worldAABB, 1, true, r_showAABB.GetInteger() == 1 ? true : false);
        }

        if (visObject->def->state.numJoints > 0 && r_showSkeleton.GetInteger() > 0) {
            DebugJoints(visObject->def, r_showSkeleton.GetInteger() == 2, camera->def->GetState().axis);
        }

        return true;
    };

    if (camera->def->GetState().orthogonal) {
        lightDbvt.Query(camera->def->box, addVisibleLights);
        objectDbvt.Query(camera->def->box, addVisibleObjects);
    } else {
        lightDbvt.Query(camera->def->frustum, addVisibleLights);
        objectDbvt.Query(camera->def->frustum, addVisibleObjects);
    }
}

// Add drawing surfaces of visible static meshes.
void RenderWorld::AddStaticMeshes(VisCamera *camera) {
    BE_PROFILE_CPU_SCOPE_STATIC("RenderWorld::AddStaticMeshes");

    // Called for each static mesh surfaces intersecting with camera frustum.
    // Returns true if it want to proceed next query.
    auto addStaticMeshSurfs = [this, camera](int32_t proxyId) -> bool {
        const DbvtProxy *proxy = (const DbvtProxy *)staticMeshDbvt.GetUserData(proxyId);
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
            float area = camera->def->state.axis[0].Abs().Dot(Vec3(size.y * size.z, size.z * size.x, size.x * size.y));
            float d = camera->def->state.origin.DistanceSqr(proxy->renderObject->state.origin);
            float lodValue = area / d;
        }*/

#if 0
        // More accurate OBB culling
        OBB obb = OBB(proxy->renderObject->GetAABB(), proxy->renderObject->state.origin, proxy->renderObject->state.axis);
        if (camera->def->frustum.CullOBB(obb)) {
            return true;
        }
#endif

        int flags = DrawSurf::Flag::Visible;
        if (proxy->renderObject->state.wireframeMode != RenderObject::WireframeMode::ShowNone || r_showWireframe.GetInteger() > 0) {
            flags |= DrawSurf::Flag::ShowWires;
        }

        VisObject *visObject = proxy->renderObject->visObject;
        AddDrawSurf(camera, nullptr, visObject, visObject->def->state.materials[surf->materialIndex], surf->subMesh, flags);

        camera->numAmbientSurfs++;

        surf->viewCount = this->viewCount;
        surf->drawSurf = camera->drawSurfs[camera->numDrawSurfs - 1];

        return true;
    };

    if (camera->def->GetState().orthogonal) {
        staticMeshDbvt.Query(camera->def->box, addStaticMeshSurfs);
    } else {
        staticMeshDbvt.Query(camera->def->frustum, addStaticMeshSurfs);
    }
}

// Add drawing surfaces of visible dynamic and skinned meshes.
void RenderWorld::AddDynamicAndSkinnedMeshes(VisCamera *camera) {
    BE_PROFILE_CPU_SCOPE_STATIC("RenderWorld::AddSkinnedMeshes");

    for (VisObject *visObject = camera->visObjects.Next(); visObject; visObject = visObject->node.Next()) {
        if (!visObject->ambientVisible) {
            continue;
        }

        const RenderObject::State &renderObjectDef = visObject->def->state;

        if (!renderObjectDef.mesh) {
            continue;
        }

#if 0
        // More accurate OBB culling
        OBB obb = OBB(visObject->def->GetAABB(), renderObjectDef.origin, renderObjectDef.axis);
        if (camera->def->frustum.CullOBB(obb)) {
            continue;
        }
#endif

        int flags = DrawSurf::Flag::Visible;
        if (renderObjectDef.wireframeMode != RenderObject::WireframeMode::ShowNone || r_showWireframe.GetInteger() > 0) {
            flags |= DrawSurf::Flag::ShowWires;
        }

        if (renderObjectDef.skeleton && renderObjectDef.joints) {
            // Update skinning joint cache for GPU skinning.
            renderObjectDef.mesh->UpdateSkinningJointCache(renderObjectDef.skeleton, renderObjectDef.joints);
        }

        for (int surfaceIndex = 0; surfaceIndex < renderObjectDef.mesh->NumSurfaces(); surfaceIndex++) {
            MeshSurf *surf = renderObjectDef.mesh->GetSurface(surfaceIndex);

            AddDrawSurf(camera, nullptr, visObject, renderObjectDef.materials[surf->materialIndex], surf->subMesh, flags);

            camera->numAmbientSurfs++;

            surf->viewCount = viewCount;
            surf->drawSurf = camera->drawSurfs[camera->numDrawSurfs - 1];
        }
    }
}

void RenderWorld::AddRawMeshes(VisCamera *camera) {
    BE_PROFILE_CPU_SCOPE_STATIC("RenderWorld::AddRawMeshes");

    for (VisObject *visObject = camera->visObjects.Next(); visObject; visObject = visObject->node.Next()) {
        if (!visObject->ambientVisible) {
            continue;
        }

        const RenderObject::State &renderObjectDef = visObject->def->state;

        if (!renderObjectDef.numVerts || !renderObjectDef.numIndexes) {
            continue;
        }

        int flags = DrawSurf::Flag::Visible;
        if (renderObjectDef.wireframeMode != RenderObject::WireframeMode::ShowNone || r_showWireframe.GetInteger() > 0) {
            flags |= DrawSurf::Flag::ShowWires;
        }

        // Cache vertices.
        BufferCache vertexCache;
        bufferCacheManager.AllocVertex(renderObjectDef.numVerts, sizeof(VertexGeneric), nullptr, &vertexCache);
        VertexGeneric *vertexPointer = (VertexGeneric *)bufferCacheManager.MapVertexBuffer(&vertexCache);
        
        simdProcessor->Memcpy(vertexPointer, renderObjectDef.verts, sizeof(renderObjectDef.verts[0]) * renderObjectDef.numVerts);
        
        bufferCacheManager.UnmapVertexBuffer(&vertexCache);

        // Cache indexes.
        BufferCache indexCache;
        bufferCacheManager.AllocIndex(renderObjectDef.numIndexes, sizeof(VertIndex), nullptr, &indexCache);
        VertIndex *indexPointer = (VertIndex *)bufferCacheManager.MapIndexBuffer(&indexCache);

        int baseVertexIndex = vertexCache.offset / sizeof(VertexGeneric);

        for (int i = 0; i < renderObjectDef.numIndexes; i++) {
            *indexPointer++ = baseVertexIndex + renderObjectDef.indexes[i];
        }

        bufferCacheManager.UnmapIndexBuffer(&indexCache);

        // Copy this SubMesh to the temporary frame data to use in the backend.
        SubMesh *subMesh = (SubMesh *)frameData.ClearedAlloc(sizeof(SubMesh));
        new (subMesh) SubMesh();

        subMesh->type = Mesh::Type::Dynamic;
        subMesh->numIndexes = renderObjectDef.numIndexes;
        subMesh->numVerts = renderObjectDef.numVerts;

        subMesh->vertexCache = (BufferCache *)frameData.Alloc(sizeof(BufferCache));
        subMesh->indexCache = (BufferCache *)frameData.Alloc(sizeof(BufferCache));

        *(subMesh->vertexCache) = vertexCache;
        *(subMesh->indexCache) = indexCache;

        AddDrawSurf(camera, nullptr, visObject, renderObjectDef.materials[0], subMesh, flags);

        camera->numAmbientSurfs++;
    }
}

// Add drawing surfaces of visible particle meshes.
void RenderWorld::AddParticleMeshes(VisCamera *camera) {
    BE_PROFILE_CPU_SCOPE_STATIC("RenderWorld::AddParticleMeshes");

    for (VisObject *visObject = camera->visObjects.Next(); visObject; visObject = visObject->node.Next()) {
        if (!visObject->ambientVisible) {
            continue;
        }

        const RenderObject::State &renderObjectDef = visObject->def->state;

        if (!renderObjectDef.particleSystem) {
            continue;
        }

        int flags = DrawSurf::Flag::Visible | DrawSurf::Flag::SkipSelection;
        if (renderObjectDef.wireframeMode != RenderObject::WireframeMode::ShowNone || r_showWireframe.GetInteger() > 0) {
            flags |= DrawSurf::Flag::ShowWires;
        }

        particleMesh.Clear();
        particleMesh.Draw(renderObjectDef.particleSystem, renderObjectDef.stageParticles, visObject->def, camera->def);
        particleMesh.CacheIndexes();

        for (int surfaceIndex = 0; surfaceIndex < particleMesh.surfaces.Count(); surfaceIndex++) {
            const PrtMeshSurf *prtMeshSurf = &particleMesh.surfaces[surfaceIndex];
            if (!prtMeshSurf->numIndexes) {
                break;
            }

            // Copy this SubMesh to the temporary frame data to use in backend.
            SubMesh *subMesh = (SubMesh *)frameData.ClearedAlloc(sizeof(SubMesh));
            new (subMesh) SubMesh();

            subMesh->type = Mesh::Type::Dynamic;
            subMesh->numIndexes = prtMeshSurf->numIndexes;
            subMesh->numVerts = prtMeshSurf->numVerts;

            subMesh->vertexCache = (BufferCache *)frameData.Alloc(sizeof(BufferCache));
            subMesh->indexCache = (BufferCache *)frameData.Alloc(sizeof(BufferCache));

            *(subMesh->vertexCache) = prtMeshSurf->vertexCache;
            *(subMesh->indexCache) = prtMeshSurf->indexCache;

            AddDrawSurf(camera, nullptr, visObject, prtMeshSurf->material, subMesh, flags);

            camera->numAmbientSurfs++;
        }
    }
}

// Add drawing surfaces of visible text meshes.
void RenderWorld::AddTextMeshes(VisCamera *camera) {
    BE_PROFILE_CPU_SCOPE_STATIC("RenderWorld::AddTextMeshes");

    for (VisObject *visObject = camera->visObjects.Next(); visObject; visObject = visObject->node.Next()) {
        if (!visObject->ambientVisible) {
            continue;
        }

        const RenderObject::State &renderObjectDef = visObject->def->state;

        if (renderObjectDef.text.IsEmpty()) {
            continue;
        }

        int flags = DrawSurf::Flag::Visible;
        if (renderObjectDef.wireframeMode != RenderObject::WireframeMode::ShowNone || r_showWireframe.GetInteger() > 0) {
            flags |= DrawSurf::Flag::ShowWires;
        }

        Color4 textColor(&renderObjectDef.materialParms[RenderObject::MaterialParm::Red]);

        textMesh.Clear();
        textMesh.SetColor(textColor);

        if (renderObjectDef.textDrawMode == RenderObject::TextDrawMode::DropShadows ||
            renderObjectDef.textDrawMode == RenderObject::TextDrawMode::AddOutlines) {
            textMesh.SetTextBorderColor(Color4(renderObjectDef.textFxColor.ToColor3(), renderObjectDef.textFxColor.a * textColor.a));
        }

        if (renderObjectDef.textRect != RectF::zero) {
            textMesh.DrawTextRect(renderObjectDef.font, renderObjectDef.textDrawMode, renderObjectDef.textRect, 
                renderObjectDef.textHorzAlignment, renderObjectDef.textVertAlignment,
                renderObjectDef.textHorzOverflow, renderObjectDef.textVertOverflow,
                renderObjectDef.lineSpacing, renderObjectDef.textScale, renderObjectDef.text);
        } else {
            textMesh.DrawText(renderObjectDef.font, renderObjectDef.textDrawMode,
                renderObjectDef.textAnchor, renderObjectDef.textHorzAlignment,
                renderObjectDef.lineSpacing, renderObjectDef.textScale, renderObjectDef.text);
        }
        textMesh.CacheIndexes();

        for (int surfaceIndex = 0; surfaceIndex < textMesh.NumSurfaces(); surfaceIndex++) {
            const GuiMeshSurf *guiMeshSurf = textMesh.Surface(surfaceIndex);
            if (!guiMeshSurf->numIndexes) {
                break;
            }

            // Copy this SubMesh to the temporary frame data to use in backend.
            SubMesh *subMesh = (SubMesh *)frameData.ClearedAlloc(sizeof(SubMesh));
            new (subMesh) SubMesh();

            subMesh->type = Mesh::Type::Dynamic;
            subMesh->numIndexes = guiMeshSurf->numIndexes;
            subMesh->numVerts = guiMeshSurf->numVerts;

            subMesh->vertexCache = (BufferCache *)frameData.Alloc(sizeof(BufferCache));
            subMesh->indexCache = (BufferCache *)frameData.Alloc(sizeof(BufferCache));

            *(subMesh->vertexCache) = guiMeshSurf->vertexCache;
            *(subMesh->indexCache) = guiMeshSurf->indexCache;

            AddDrawSurf(camera, nullptr, visObject, guiMeshSurf->material, subMesh, flags);

            camera->numAmbientSurfs++;
        }
    }
}

// Add drawing surfaces of the skybox mesh.
void RenderWorld::AddSkyBoxMeshes(VisCamera *camera) {
    BE_PROFILE_CPU_SCOPE_STATIC("RenderWorld::AddSkyBoxMeshes");

    if (camera->def->state.clearMethod != RenderCamera::ClearMethod::Skybox) {
        return;
    }

    // Skybox object parameters.
    RenderObject::State roDef;
    roDef.staticMask = -1;
    roDef.flags = RenderObject::Flag::SkipSelection;
    roDef.worldMatrix.SetTRS(camera->def->GetState().origin, Mat3::identity, Vec3(10, 10, 10));
    roDef.materialParms[RenderObject::MaterialParm::Red] = 1.0f;
    roDef.materialParms[RenderObject::MaterialParm::Green] = 1.0f;
    roDef.materialParms[RenderObject::MaterialParm::Blue] = 1.0f;
    roDef.materialParms[RenderObject::MaterialParm::Alpha] = 1.0f;
    roDef.materialParms[RenderObject::MaterialParm::TimeScale] = 1.0f;

    static RenderObject renderObject(this, -1);
    new (&renderObject) RenderObject(this, -1);
    renderObject.Update(&roDef);

    // Add skybox object.
    VisObject *visObject = RegisterVisObject(camera, &renderObject);

    if (camera->def->GetState().orthogonal) {
        ALIGN_AS32 Mat4 projMatrix;
        R_SetPerspectiveProjectionMatrix(45, 45, 0.01, 1000, false, projMatrix);

        visObject->modelViewMatrix = camera->def->viewMatrix * renderObject.GetWorldMatrix();
        visObject->modelViewProjMatrix = projMatrix * visObject->modelViewMatrix;
    } else {
        visObject->modelViewMatrix = camera->def->viewMatrix * renderObject.GetWorldMatrix();
        visObject->modelViewProjMatrix = camera->def->viewProjMatrix * renderObject.GetWorldMatrix();
    }

    MeshSurf *meshSurf = meshManager.defaultBoxMesh->GetSurface(0);
    AddDrawSurf(camera, nullptr, visObject, skyboxMaterial, meshSurf->subMesh, DrawSurf::Flag::Visible);

    camera->numAmbientSurfs++;
}

// Add lit drawing surfaces of visible static meshes for each light.
void RenderWorld::AddStaticMeshesForLights(VisCamera *camera) {
    BE_PROFILE_CPU_SCOPE_STATIC("RenderWorld::AddStaticMeshesForLights");

    VisLight *visLight;

    // Called for static mesh surfaces intersecting with each light volumes.
    // Returns true if it want to proceed next query.
    auto addStaticMeshSurfsForLights = [this, camera, &visLight](int32_t proxyId) -> bool {
        const DbvtProxy *proxy = (const DbvtProxy *)staticMeshDbvt.GetUserData(proxyId);
        RenderObject *renderObject = proxy->renderObject;

        MeshSurf *surf = proxy->mesh->GetSurface(proxy->meshSurfIndex);

        if (!surf) {
            return true;
        }

        if (renderObject->state.flags & RenderObject::Flag::SkipRendering) {
            return true;
        }

        // Skip if object layer is not visible with this camera.
        if (!(BIT(renderObject->state.layer) & camera->def->GetState().layerMask)) {
            return true;
        }

        // Skip if camera renders static objects and this object is not static.
        if (camera->def->GetState().flags & RenderCamera::Flag::StaticOnly) {
            if (!(renderObject->state.staticMask & camera->def->GetState().staticMask)) {
                return true;
            }
        }

        // Skip first person camera only object in sub camera.
        if ((renderObject->state.flags & RenderObject::Flag::FirstPersonOnly) && camera->isSubCamera) {
            return true;
        }

        // Skip 3rd person camera only object in sub camera.
        if ((renderObject->state.flags & RenderObject::Flag::ThirdPersonOnly) && !camera->isSubCamera) {
            return true;
        }

        // Skip if the object is farther than maximum visible distance.
        if (!(renderObject->state.flags & RenderObject::Flag::NoVisDist)) {
            if (renderObject->state.worldMatrix.ToTranslationVec3().DistanceSqr(camera->def->state.origin) > renderObject->maxVisDistSquared) {
                return true;
            }
        }

        const Material *material = renderObject->state.materials[surf->materialIndex];

        bool isShadowCaster = (visLight->def->state.flags & RenderLight::Flag::CastShadows) && (renderObject->state.flags & RenderObject::Flag::CastShadows) && material->IsShadowCaster();

        // Already visible in this frame.
        if (surf->viewCount == this->viewCount) {
            if ((surf->drawSurf->flags & DrawSurf::Flag::Visible) && material->IsLitSurface()) {
                // Add drawSurf from visible drawSurf.
                AddDrawSurfFromAmbient(camera, visLight, isShadowCaster, surf->drawSurf);

                visLight->numDrawSurfs++;
                visLight->litSurfsAABB.AddAABB(proxy->worldAABB);

                if (isShadowCaster) {
                    visLight->shadowCastersAABB.AddAABB(proxy->worldAABB);
                }
            }
        } else if (isShadowCaster) {
            OBB surfBounds = OBB(surf->subMesh->GetAABB(), renderObject->state.worldMatrix);

            if (!visLight->def->CullShadowCaster(surfBounds, camera->def->frustum, camera->worldAABB)) {
                // This surface is not visible but shadow might be visible as a shadow caster.
                // Register a visObject used only for shadow caster.
                VisObject *shadowCasterObject = RegisterVisObject(camera, renderObject);
                shadowCasterObject->shadowVisible = true;

                AddDrawSurf(camera, visLight, shadowCasterObject, material, surf->subMesh, DrawSurf::Flag::ShadowVisible);

                surf->viewCount = this->viewCount;
                surf->drawSurf = camera->drawSurfs[camera->numDrawSurfs - 1];

                visLight->numDrawSurfs++;
                visLight->shadowCastersAABB.AddAABB(proxy->worldAABB);
            }
        }

        return true;
    };

    for (visLight = camera->visLights.Next(); visLight; visLight = visLight->node.Next()) {
        const RenderLight *renderLight = visLight->def;

        if (!(BIT(visLight->def->state.layer) & camera->def->GetState().layerMask)) {
            continue;
        }

        switch (renderLight->state.type) {
        case RenderLight::Type::Directional:
            staticMeshDbvt.Query(renderLight->worldOBB, addStaticMeshSurfsForLights);
            break;
        case RenderLight::Type::Point:
            if (renderLight->IsRadiusUniform()) {
                staticMeshDbvt.Query(Sphere(renderLight->GetOrigin(), renderLight->GetRadius()[0]), addStaticMeshSurfsForLights);
            } else {
                staticMeshDbvt.Query(renderLight->worldOBB, addStaticMeshSurfsForLights);
            }
            break;
        case RenderLight::Type::Spot:
            staticMeshDbvt.Query(renderLight->worldFrustum, addStaticMeshSurfsForLights);
            break;
        default:
            break;
       }
    }
}

// Add lit drawing surfaces of visible dynamic and skinned meshes for each light.
void RenderWorld::AddDynamicAndSkinnedMeshesForLights(VisCamera *camera) {
    BE_PROFILE_CPU_SCOPE_STATIC("RenderWorld::AddSkinnedMeshesForLights");

    VisLight *visLight;

    // Called for entities intersecting with each light volume.
    // Returns true if it want to proceed next query.
    auto addShadowCasterObjects = [this, camera, &visLight](int32_t proxyId) -> bool {
        const DbvtProxy *proxy = (const DbvtProxy *)this->objectDbvt.GetUserData(proxyId);
        RenderObject *renderObject = proxy->renderObject;

        if (!renderObject) {
            return true;
        }

        if (renderObject->state.flags & RenderObject::Flag::SkipRendering) {
            return true;
        }

        // Skip if object layer is not visible with this camera.
        if (!(BIT(renderObject->state.layer) & camera->def->GetState().layerMask)) {
            return true;
        }

        // Skip if camera renders static objects and this object is not static.
        if (camera->def->GetState().flags & RenderCamera::Flag::StaticOnly) {
            if (!(renderObject->state.staticMask & camera->def->GetState().staticMask)) {
                return true;
            }
        }

        // Skip first person camera only object in sub camera.
        if ((renderObject->state.flags & RenderObject::Flag::FirstPersonOnly) && camera->isSubCamera) {
            return true;
        }

        // Skip 3rd person camera only object in sub camera.
        if ((renderObject->state.flags & RenderObject::Flag::ThirdPersonOnly) && !camera->isSubCamera) {
            return true;
        }

        bool isShadowCaster = (visLight->def->state.flags & RenderLight::Flag::CastShadows) && (renderObject->state.flags & RenderObject::Flag::CastShadows);
        bool shadowCasterCulled = false;

        if (isShadowCaster && !renderObject->visObject) {
            OBB worldOBB = renderObject->GetWorldOBB();

            shadowCasterCulled = visLight->def->CullShadowCaster(worldOBB, camera->def->frustum, camera->worldAABB);
        }

        VisObject *shadowCasterObject = nullptr;

        for (int surfaceIndex = 0; surfaceIndex < renderObject->state.mesh->NumSurfaces(); surfaceIndex++) {
            MeshSurf *surf = renderObject->state.mesh->GetSurface(surfaceIndex);

            const Material *material = renderObject->state.materials[surf->materialIndex];

            // Already visible in this frame.
            if (surf->viewCount == this->viewCount) {
                if ((surf->drawSurf->flags & DrawSurf::Flag::Visible) && material->IsLitSurface()) {
                    // Add drawSurf from visible drawSurf.
                    AddDrawSurfFromAmbient(camera, visLight, isShadowCaster && material->IsShadowCaster(), surf->drawSurf);

                    visLight->numDrawSurfs++;
                    visLight->litSurfsAABB.AddAABB(proxy->worldAABB);

                    if (isShadowCaster && material->IsShadowCaster()) {
                        visLight->shadowCastersAABB.AddAABB(proxy->worldAABB);
                    }
                }
            } else if (isShadowCaster && material->IsShadowCaster()) {
                if (!shadowCasterCulled) {
                    // This surface is not visible but shadow might be visible as a shadow caster.
                    // Register a visObject used only for shadow caster.
                    if (!shadowCasterObject) {
                        shadowCasterObject = RegisterVisObject(camera, renderObject);
                        shadowCasterObject->shadowVisible = true;
                    }

                    if (shadowCasterObject->def->state.skeleton && shadowCasterObject->def->state.joints) {
                        shadowCasterObject->def->state.mesh->UpdateSkinningJointCache(shadowCasterObject->def->state.skeleton, shadowCasterObject->def->state.joints);
                    }

                    AddDrawSurf(camera, visLight, shadowCasterObject, material, surf->subMesh, DrawSurf::Flag::ShadowVisible);

                    surf->viewCount = this->viewCount;
                    surf->drawSurf = camera->drawSurfs[camera->numDrawSurfs - 1];

                    visLight->numDrawSurfs++;
                    visLight->shadowCastersAABB.AddAABB(proxy->worldAABB);
                }
            }
        }

        return true;
    };
    
    for (visLight = camera->visLights.Next(); visLight; visLight = visLight->node.Next()) {
        const RenderLight *renderLight = visLight->def;

        if (!(BIT(visLight->def->state.layer) & camera->def->GetState().layerMask)) {
            continue;
        }

        switch (renderLight->state.type) {
        case RenderLight::Type::Directional:
            objectDbvt.Query(renderLight->worldOBB, addShadowCasterObjects);
            break;
        case RenderLight::Type::Point:
            if (renderLight->IsRadiusUniform()) {
                objectDbvt.Query(Sphere(renderLight->GetOrigin(), renderLight->GetRadius()[0]), addShadowCasterObjects);
            } else {
                objectDbvt.Query(renderLight->worldOBB, addShadowCasterObjects);
            }
            break;
        case RenderLight::Type::Spot:
            objectDbvt.Query(renderLight->worldFrustum, addShadowCasterObjects);
            break;
        default:
            break;
        }
    }
}

void RenderWorld::CacheInstanceBuffer(VisCamera *camera) {
    BE_PROFILE_CPU_SCOPE_STATIC("RenderWorld::CacheInstanceBuffer");

    if (renderGlobal.instancingMethod == Mesh::InstancingMethod::NoInstancing) {
        return;
    }

    int numInstances = 0;

    for (VisObject *visObject = camera->visObjects.Next(); visObject; visObject = visObject->node.Next()) {
        const RenderObject *renderObject = visObject->def;

        // Only for mesh type render object.
        if (!renderObject->state.mesh) {
            continue;
        }

        for (int surfaceIndex = 0; surfaceIndex < renderObject->state.mesh->NumSurfaces(); surfaceIndex++) {
            const MeshSurf *surf = renderObject->state.mesh->GetSurface(surfaceIndex);

            if (surf->viewCount != viewCount) {
                continue;
            }

            if (!(surf->drawSurf->flags & DrawSurf::Flag::UseInstancing)) {
                continue;
            }

            byte *instanceData = ((byte *)renderGlobal.instanceBufferData + numInstances * renderGlobal.instanceBufferOffsetAlignment);

            const Mat3x4 &localToWorldMatrix = renderObject->GetWorldMatrix();
            *(Mat3x4 *)instanceData = localToWorldMatrix;
            instanceData += 48;

            /*if (surf->drawSurf->material->GetPass()->shader->GetPropertyInfoHashMap().Get("_PARALLAX")) {
                Mat3x4 worldToLocalMatrix = renderObject->GetWorldMatrixInverse();
                *(Mat3x4 *)instanceData = worldToLocalMatrix; 
                instanceData += 48;
            }*/

            if (renderGlobal.instancingMethod == Mesh::InstancingMethod::InstancedArrays) {
                if (surf->drawSurf->material->GetPass()->useOwnerColor) {
                    *(uint32_t *)instanceData = Color4(&renderObject->state.materialParms[RenderObject::MaterialParm::Red]).ToUInt32();
                } else {
                    *(uint32_t *)instanceData = surf->drawSurf->material->GetPass()->constantColor.ToUInt32();
                }
                instanceData += sizeof(uint32_t);
            } else {
                if (surf->drawSurf->material->GetPass()->useOwnerColor) {
                    *(Color4 *)instanceData = Color4(&renderObject->state.materialParms[RenderObject::MaterialParm::Red]);
                } else {
                    *(Color4 *)instanceData = surf->drawSurf->material->GetPass()->constantColor;
                }
                instanceData += sizeof(Color4);
            }

            if (surf->subMesh->IsGpuSkinningEnabled()) {
                const SkinningJointCache *skinningJointCache = renderObject->state.mesh->skinningJointCache;

                if (renderGlobal.vertexTextureMethod == BufferCacheManager::VertexTextureMethod::Tbo) {
                    *(uint32_t *)instanceData = (uint32_t)skinningJointCache->GetBufferCache().tcBase[0];
                } else {
                    *(Vec2 *)instanceData = Vec2(skinningJointCache->GetBufferCache().tcBase[0], skinningJointCache->GetBufferCache().tcBase[1]);
                }
            }

            visObject->instanceIndex = numInstances++;
            break;
        }
    }

    if (numInstances > 0) {
        if (renderGlobal.instancingMethod == Mesh::InstancingMethod::InstancedArrays) {
            bufferCacheManager.AllocVertex(numInstances, renderGlobal.instanceBufferOffsetAlignment, renderGlobal.instanceBufferData, camera->instanceBufferCache);
        } else if (renderGlobal.instancingMethod == Mesh::InstancingMethod::UniformBuffer) {
            bufferCacheManager.AllocUniform(numInstances * renderGlobal.instanceBufferOffsetAlignment, renderGlobal.instanceBufferData, camera->instanceBufferCache);
        }
    }
}

void RenderWorld::OptimizeLights(VisCamera *camera) {
    LinkList<VisLight> *nextNode;

    // Iterate over light linked list in camera.
    for (LinkList<VisLight> *node = camera->visLights.NextNode(); node; node = nextNode) {
        VisLight *visLight = node->Owner();

        nextNode = node->NextNode();

        if (visLight->numDrawSurfs == 0) {
            node->Remove();

            camera->numVisibleLights--;
            continue;
        }

        const AABB lightAABB = visLight->def->GetWorldAABB();

        // Compute effective AABB.
        visLight->litSurfsAABB.IntersectSelf(lightAABB);
        visLight->shadowCastersAABB.IntersectSelf(lightAABB);
        
        if (visLight->def->state.flags & RenderLight::Flag::PrimaryLight) {
            camera->primaryLight = visLight;
            continue;
        }

        Rect screenClipRect;
        if (!camera->def->CalcClipRectFromAABB(visLight->litSurfsAABB, screenClipRect)) {
            node->Remove();

            camera->numVisibleLights--;
            continue;
        }

        Rect visScissorRect = screenClipRect;
        visScissorRect.y = camera->def->state.renderRect.h - (screenClipRect.y + screenClipRect.h);
        visLight->scissorRect.IntersectSelf(visScissorRect);

        if (visLight->scissorRect.IsEmpty()) {
            node->Remove();

            camera->numVisibleLights--;
            continue;
        }
    }
}

void RenderWorld::DrawCamera(VisCamera *camera) {
    BE_PROFILE_CPU_SCOPE_STATIC("RenderWorld::DrawCamera");

    viewCount++;

    // Find visible renderLights by querying view frustum in lightDBVT.
    // Then register each visible renderLight to the current camera as VisLight.
    // Find visible renderObjects by querying view frustum in objectDBVT.
    // Then register each visible renderObject to the current camera as VisObject.
    FindVisLightsAndObjects(camera);

    // Add drawing surfaces of static meshes by querying view frustum in staticMeshDBVT.
    AddStaticMeshes(camera);

    // Add drawing surfaces of skinned meshes by searching in visObjects.
    AddDynamicAndSkinnedMeshes(camera);

    //AddSubCamera(camera);

    // Add drawing surfaces of raw meshes by searching in visObjects.
    AddRawMeshes(camera);

    // Add drawing surfaces of text meshes by searching in visObjects.
    AddTextMeshes(camera);

    // Add drawing surfaces of particle meshes by searching in visObjects.
    AddParticleMeshes(camera);

    // Add drawing surface of skybox.
    AddSkyBoxMeshes(camera);
    
    // Add drawing surfaces of static meshes by querying light BV in staticMeshDBVT.
    // Added drawing surface might be the shadow caster only surface if it is not the visible in the previous steps.
    AddStaticMeshesForLights(camera);

    // Add drawing surfaces of skinned meshes by querying light BV in objectMeshDBVT.
    // Added drawing surface might be the shadow caster only surface if it is not the visible in the previous steps.
    AddDynamicAndSkinnedMeshesForLights(camera);

    // Compute scissor rect of each visLights and exclude if it is not visible.
    OptimizeLights(camera);

    // Cache instance data for instancing.
    CacheInstanceBuffer(camera);
    
    // Sort drawing surfaces.
    SortDrawSurfs(camera);

    renderSystem.CmdDrawCamera(camera);
}

void RenderWorld::AddSubCamera(VisCamera *camera) {
    if (!(camera->def->GetState().flags & RenderCamera::Flag::NoSubViews)) {
        for (int i = 0; i < camera->numDrawSurfs; i++) {
            DrawSurf *drawSurf = camera->drawSurfs[i];

            if (!(drawSurf->flags & DrawSurf::Flag::Visible)) {
                continue;
            }

            if (drawSurf->material->GetSort() > Material::Sort::SubView) {
                break;
            }

            if (drawSurf->material->GetSort() == Material::Sort::Bad) {
                BE_ERRLOG("Material '%s' with sort == Sort::Bad\n", drawSurf->material->GetName());
            }

            DrawSubCamera(drawSurf->space, drawSurf, drawSurf->material);

            if (r_subViewOnly.GetBool()) {
                break;
            }
        }
    }
}

void RenderWorld::DrawSubCamera(const VisObject *visObject, const DrawSurf *drawSurf, const Material *material) {
    BE_PROFILE_CPU_SCOPE_STATIC("RenderWorld::DrawSubCamera");
}

void RenderWorld::AddDrawSurf(VisCamera *camera, VisLight *visLight, VisObject *visObject, const Material *material, SubMesh *subMesh, int flags) {
    if (camera->numDrawSurfs + 1 > camera->maxDrawSurfs) {
        BE_WARNLOG("RenderWorld::AddDrawSurf: not enough renderable surfaces\n");
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
    localParms[0] = camera->def->state.time * 0.001f;
    memcpy(&localParms[1], visObject->def->state.materialParms, sizeof(visObject->def->state.materialParms));

    actualMaterial->GetExprChunk()->Evaluate(localParms, outputValues);*/

    if (!bufferCacheManager.IsCached(subMesh->vertexCache)) {
        Mesh::Type::Enum subMeshType = (Mesh::Type::Enum)subMesh->GetType();

        if (subMeshType == Mesh::Type::Reference ||
            subMeshType == Mesh::Type::Static ||
            subMeshType == Mesh::Type::Skinned) {
            subMesh->CacheStaticDataToGpu();
        } else if (subMeshType == Mesh::Type::Dynamic) {
            subMesh->CacheDynamicDataToGpu(visObject->def->state.joints, actualMaterial);
        } else {
            assert(0);
        }
    }

    if (renderGlobal.instancingMethod != Mesh::InstancingMethod::NoInstancing) {
        if (actualMaterial->GetPass()->instancingEnabled) {
            if (subMesh->IsGpuSkinningEnabled()) {
                if (renderGlobal.skinningMethod == SkinningJointCache::SkinningMethod::VertexTextureFetch) {
                    flags |= DrawSurf::Flag::UseInstancing;
                }
            } else {
                flags |= DrawSurf::Flag::UseInstancing;
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
    uint64_t materialIndex = materialManager.GetIndexByMaterial(actualMaterial);

    if (visObject->def->state.flags & RenderObject::Flag::UseRenderingOrder) {
        //---------------------------------------------------
        // 0xFFF0000000000000 (0~4095)  : visLight index
        // 0x000F000000000000 (0~15)    : material sort (Material::Sort::Overlay)
        // 0x0000FFFF00000000 (0~65535) : rendering order
        // 0x00000000FFFF0000 (0~65535) : material index
        // 0x000000000000FFFF (0~65535) : visObject index
        //---------------------------------------------------
        uint64_t materialSort = Material::Sort::Overlay;
        uint64_t renderingOrder = visObject->def->state.renderingOrder & 0xFFFF;

        drawSurf->sortKey = ((visLightIndex << 52) | (materialSort << 48) | (renderingOrder << 32) | (materialIndex << 16) | visObjectIndex);
    } else {
        uint64_t materialSort = actualMaterial->GetSort();

        if (materialSort == Material::Sort::Translucent || materialSort == Material::Sort::Overlay) {
            // Rough sorting back-to-front order for translucent surfaces.
            float depthMin = 0.0f;
            float depthMax = 1.0f;

            camera->def->CalcDepthBoundsFromAABB(subMesh->GetAABB(), visObject->modelViewProjMatrix, &depthMin, &depthMax);

            uint64_t depthDist = Math::Ftoui16(depthMin * 0xFFFF);
            depthDist = 0xFFFF - depthDist;

            //---------------------------------------------------
            // 0xFFF0000000000000 (0~4095)  : visLight index
            // 0x000F000000000000 (0~15)    : material sort
            // 0x0000FFFF00000000 (0~65535) : depth distance
            // 0x00000000FFFF0000 (0~65535) : material index
            // 0x000000000000FFFF (0~65535) : visObject index
            //---------------------------------------------------
            drawSurf->sortKey = ((visLightIndex << 52) | (materialSort << 48) | (depthDist << 32) | (materialIndex << 16) | visObjectIndex);
        } else {
            uint64_t subMeshIndex = ((uint64_t)subMesh->subMeshIndex & 0xFFFF);

            //---------------------------------------------------
            // 0xFFF0000000000000 (0~4095)  : visLight index
            // 0x000F000000000000 (0~15)    : material sort
            // 0x0000FFFF00000000 (0~65535) : sub mesh index
            // 0x00000000FFFF0000 (0~65535) : material index
            // 0x000000000000FFFF (0~65535) : visObject index
            //---------------------------------------------------
            drawSurf->sortKey = ((visLightIndex << 52) | (materialSort << 48) | (subMeshIndex << 32) | (materialIndex << 16) | visObjectIndex);
        }
    }

    camera->drawSurfs[camera->numDrawSurfs++] = drawSurf;
}

void RenderWorld::AddDrawSurfFromAmbient(VisCamera *camera, const VisLight *visLight, bool shadowVisible, const DrawSurf *visibleDrawSurf) {
    if (camera->numDrawSurfs + 1 > camera->maxDrawSurfs) {
        BE_WARNLOG("RenderWorld::AddDrawSurfFromAmbient: not enough renderable surfaces\n");
        return;
    }

    DrawSurf *drawSurf = (DrawSurf *)frameData.Alloc(sizeof(DrawSurf));
    drawSurf->sortKey = (visibleDrawSurf->sortKey & 0x000FFFFFFFFFFFFF) | ((uint64_t)(visLight->index + 1) << 52);
    drawSurf->flags = visibleDrawSurf->flags | (shadowVisible ? DrawSurf::Flag::ShadowVisible : 0);
    drawSurf->space = visibleDrawSurf->space;
    drawSurf->material = visibleDrawSurf->material;
    drawSurf->materialRegisters = visibleDrawSurf->materialRegisters;
    drawSurf->subMesh = visibleDrawSurf->subMesh;

    camera->drawSurfs[camera->numDrawSurfs++] = drawSurf;
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

void RenderWorld::SortDrawSurfs(VisCamera *camera) {
    BE_PROFILE_CPU_SCOPE_STATIC("RenderWorld::SortDrawSurfs");

    // FIXME: Use radix sort to boost up speed.
    qsort(camera->drawSurfs, camera->numDrawSurfs, sizeof(DrawSurf *), _CompareDrawSurf);

    VisLight *visLight = camera->visLights.Next();

    for (int i = 0; i < camera->numDrawSurfs; i++) {
        if (!visLight) {
            break;
        }
        
        int lightIndex = (camera->drawSurfs[i]->sortKey & 0xFFF0000000000000) >> 52;

        if (lightIndex == visLight->index + 1) {
            visLight->firstDrawSurf = i;
            assert(i + visLight->numDrawSurfs <= camera->numDrawSurfs);

            visLight = visLight->node.Next();
        }
    }
}

BE_NAMESPACE_END
