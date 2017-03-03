#include "Precompiled.h"
#include "Render/Render.h"
#include "RenderInternal.h"

BE_NAMESPACE_BEGIN

// sceneEntity 로 부터 viewEntity 를 등록, 같은 view 에 여러번 등록 방지
viewEntity_t *RenderWorld::AddViewEntity(view_t *view, SceneEntity *sceneEntity) {
    if (sceneEntity->viewCount == viewCount) {
        return sceneEntity->viewEntity;
    }

    viewEntity_t *viewEntity = (viewEntity_t *)frameData.ClearedAlloc(sizeof(*viewEntity));
    viewEntity->def = sceneEntity;

    // view 에 linked list 로 연결
    viewEntity->next = view->viewEntities;
    view->viewEntities = viewEntity;

    // viewEntity 를 sceneEntity 에 연결
    sceneEntity->viewEntity = viewEntity;
    sceneEntity->viewCount = viewCount;

    return viewEntity;
}

// sceneLight 로 부터 viewLight 를 등록, 같은 view 에 여러번 등록 방지
viewLight_t *RenderWorld::AddViewLight(view_t *view, SceneLight *sceneLight) {
    if (sceneLight->viewCount == viewCount) {
        return sceneLight->viewLight;
    }
    
    viewLight_t *viewLight = (viewLight_t *)frameData.ClearedAlloc(sizeof(*viewLight));
    viewLight->def = sceneLight;

    // view 에 linked list 로 연결
    viewLight->next = view->viewLights;
    view->viewLights = viewLight;

    // viewLight 를 sceneLight 에 연결
    sceneLight->viewLight = viewLight;
    sceneLight->viewCount = viewCount;

    return viewLight;
}

// viewLight 와 viewEntity 들을 등록한다.
void RenderWorld::AddViewLightsAndEntities(view_t *view) {
    viewCount++;

    view->aabb.Clear();
    view->viewLights = nullptr;
    view->viewEntities = nullptr;

    // Called for each entities and lights intersecting with view frustum 
    auto addViewLightsAndEntities = [this, view](int32_t proxyId) -> bool {
        DbvtProxy *proxy = (DbvtProxy *)dynamicDbvt.GetUserData(proxyId);
        if (proxy->sceneLight) {
            SceneLight *sceneLight = proxy->sceneLight;

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
            if (sceneLight->Cull(view->def->frustum)) {
                return true;
            }

            // light scissorRect 계산
            Rect screenClipRect;
            if (!sceneLight->ComputeScreenClipRect(view->def, screenClipRect)) {
                return true;
            }

            viewLight_t *viewLight = this->AddViewLight(view, sceneLight);
            viewLight->scissorRect = screenClipRect;
            // glScissor 의 x, y 좌표는 lower left corner 이므로 y 좌표를 밑에서 증가되도록 뒤집는다.
            viewLight->scissorRect.y = renderSystem.currentContext->GetRenderHeight() - (screenClipRect.y + screenClipRect.h);

            viewLight->litAABB.Clear();
            viewLight->shadowCasterAABB.Clear();
        } else if (proxy->sceneEntity) {
            SceneEntity *sceneEntity = proxy->sceneEntity;

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

            viewEntity_t *viewEntity = this->AddViewEntity(view, sceneEntity);
            
            viewEntity->ambientVisible = true;

            viewEntity->modelViewMatrix = view->def->viewMatrix * sceneEntity->GetModelMatrix();
            viewEntity->modelViewProjMatrix = view->def->viewProjMatrix * sceneEntity->GetModelMatrix();

            if (sceneEntity->parms.billboard) {
                Mat3 vmt = (view->def->viewMatrix.ToMat3() * sceneEntity->GetModelMatrix().ToMat3()).Inverse();
                //vmt = vmt * Mat3(0, 0, 1, 1, 0, 0, 0, 1, 0);
                Swap(vmt[0], vmt[2]);
                Swap(vmt[1], vmt[2]);

                Mat4 billboardMatrix = (vmt * sceneEntity->parms.axis.Scale(sceneEntity->parms.scale)).ToMat4();
                viewEntity->modelViewMatrix *= billboardMatrix;
                viewEntity->modelViewProjMatrix *= billboardMatrix;
            }

            view->aabb.AddAABB(proxy->aabb);

            if (r_showAABB.GetInteger() > 0) {
                this->SetDebugColor(Color4::blue, Color4::zero);
                this->DebugAABB(proxy->aabb, 1, true, r_showAABB.GetInteger() == 1 ? true : false);
            }

            if (viewEntity->def->parms.numJoints > 0 && r_showSkeleton.GetInteger() > 0) {
                this->DebugJoints(viewEntity->def, r_showSkeleton.GetInteger() == 2, view->def->frustum.GetAxis());
            }
        }

        return true;
    };

    if (view->def->parms.orthogonal) {
        dynamicDbvt.Query(view->def->box, addViewLightsAndEntities);
    } else {
        dynamicDbvt.Query(view->def->frustum, addViewLightsAndEntities);
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

            AddDrawSurf(view, viewEntity, entityParms.customMaterials[surf->materialIndex], surf->subMesh, nullptr, flags);

            surf->viewCount = viewCount;
            surf->drawSurf = view->drawSurfs[view->numDrawSurfs - 1];
        }
    }
}

void RenderWorld::AddTextMeshes(view_t *view) {
    for (viewEntity_t *viewEntity = view->viewEntities; viewEntity; viewEntity = viewEntity->next) {
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

        textMesh.DrawText(entityParms.font, entityParms.textAnchor, entityParms.textAlignment, entityParms.lineSpacing, entityParms.textScale, entityParms.text);
        
        for (int surfaceIndex = 0; surfaceIndex < textMesh.surfaces.Count(); surfaceIndex++) {
            GuiMeshSurf *guiSurf = &textMesh.surfaces[surfaceIndex];
            if (!guiSurf->numIndexes) {
                break;
            }

            GuiSubMesh *guiSubMesh      = (GuiSubMesh *)frameData.ClearedAlloc(sizeof(GuiSubMesh));
            guiSubMesh->numIndexes      = guiSurf->numIndexes;
            guiSubMesh->numVerts        = guiSurf->numVerts;

            guiSubMesh->vertexCache     = (BufferCache *)frameData.ClearedAlloc(sizeof(BufferCache));
            *(guiSubMesh->vertexCache)  = guiSurf->vertexCache;

            guiSubMesh->indexCache      = (BufferCache *)frameData.ClearedAlloc(sizeof(BufferCache));
            *(guiSubMesh->indexCache)   = guiSurf->indexCache;

            AddDrawSurf(view, viewEntity, guiSurf->material, nullptr, guiSubMesh, flags);
        }
    }
}

// static mesh 들을 ambient drawSurfs 에 담는다. 
void RenderWorld::AddStaticMeshes(view_t *view) {
    // Called for each static mesh surfaces intersecting with view frustum 
    auto addStaticMeshSurfs = [this, view](int32_t proxyId) -> bool {
        DbvtProxy *proxy = (DbvtProxy *)staticDbvt.GetUserData(proxyId);
        MeshSurf *meshSurf = proxy->mesh->GetSurface(proxy->meshSurfIndex);

        // meshSurf 가 없다면 static mesh 이 아님
        if (!meshSurf) {
            return true;
        }

        if (proxy->sceneEntity->viewCount != this->viewCount) {
            return true;
        }

        /*if (proxy->lodGroup >= 0) {
            // Compute LOD value [0, 1]
            AABB aabb = meshSurf->subMesh->GetAABB() * proxy->sceneEntity->parms.scale;
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
        this->AddDrawSurf(view, viewEntity, viewEntity->def->parms.customMaterials[meshSurf->materialIndex], meshSurf->subMesh, nullptr, flags);

        meshSurf->viewCount = this->viewCount;
        meshSurf->drawSurf = view->drawSurfs[view->numDrawSurfs - 1];
        //bool has = meshSurf->drawSurf->material->IsLitSurface();

        if (r_showAABB.GetInteger() > 0) {
            this->SetDebugColor(Color4(1, 1, 1, 0.5), Color4::zero);
            this->DebugAABB(proxy->aabb, 1, true, r_showAABB.GetInteger() == 1 ? true : false);
        }

        return true;
    };

    if (view->def->parms.orthogonal) {
        staticDbvt.Query(view->def->box, addStaticMeshSurfs);
    } else {
        staticDbvt.Query(view->def->frustum, addStaticMeshSurfs);
    }
}

// static mesh 들을 viewLight 의 litSurfs/shadowCasterSurfs 리스트에 담는다.
void RenderWorld::AddStaticMeshesForLights(view_t *view) {
    viewLight_t *viewLight = view->viewLights;

    auto addStaticMeshSurfsForLights = [this, view, &viewLight](int32_t proxyId) -> bool {
        DbvtProxy *proxy = (DbvtProxy *)this->staticDbvt.GetUserData(proxyId);
        MeshSurf *meshSurf = proxy->mesh->GetSurface(proxy->meshSurfIndex);

        if (!meshSurf) {
            return true;
        }

        // Skip first person view only entity in subView 
        if (proxy->sceneEntity->parms.firstPersonOnly && view->isSubview) {
            return true;
        }

        // Skip 3rd person view only entity in subView
        if (proxy->sceneEntity->parms.thirdPersonOnly && !view->isSubview) {
            return true;
        }

        // Skip if a entity is farther than maximum visible distance
        if (proxy->sceneEntity->parms.origin.DistanceSqr(view->def->parms.origin) >proxy->sceneEntity->parms.maxVisDist * proxy->sceneEntity->parms.maxVisDist) {
            return true;
        }

        const Material *material = proxy->sceneEntity->parms.customMaterials[meshSurf->materialIndex];

        if (meshSurf->viewCount == this->viewCount) {
            if ((meshSurf->drawSurf->flags & DrawSurf::AmbientVisible) && material->IsLitSurface()) {
                drawSurfNode_t *drawSurfNode = (drawSurfNode_t *)frameData.Alloc(sizeof(drawSurfNode_t));
                drawSurfNode->drawSurf = meshSurf->drawSurf;
                drawSurfNode->next = viewLight->litSurfs;

                viewLight->litSurfs = drawSurfNode;
                viewLight->litAABB.AddAABB(proxy->aabb);
            }
        }
            
        if (material->IsShadowCaster()) {
            OBB obb = OBB(meshSurf->subMesh->GetAABB() * proxy->sceneEntity->parms.scale, proxy->sceneEntity->parms.origin, proxy->sceneEntity->parms.axis);
            if (viewLight->def->CullShadowCasterOBB(obb, view->def->frustum, view->aabb)) {
                return true;
            }

            if (meshSurf->viewCount != this->viewCount) {
                viewEntity_t *shadowViewEntity = this->AddViewEntity(view, proxy->sceneEntity);
                shadowViewEntity->shadowVisible = true;

                this->AddDrawSurf(view, shadowViewEntity, material, meshSurf->subMesh, nullptr, 0);

                meshSurf->viewCount = this->viewCount;
                meshSurf->drawSurf = view->drawSurfs[view->numDrawSurfs - 1];
            }

            drawSurfNode_t *drawSurfNode = (drawSurfNode_t *)frameData.Alloc(sizeof(drawSurfNode_t));
            drawSurfNode->drawSurf = meshSurf->drawSurf;
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
            staticDbvt.Query(sceneLight->obb, addStaticMeshSurfsForLights);
            break;
        case SceneLight::PointLight:
            if (sceneLight->IsRadiusUniform()) {
                staticDbvt.Query(Sphere(sceneLight->GetOrigin(), sceneLight->GetRadius()[0]), addStaticMeshSurfsForLights);
            } else {
                staticDbvt.Query(sceneLight->obb, addStaticMeshSurfsForLights);
            }
            break;
        case SceneLight::SpotLight:
            staticDbvt.Query(sceneLight->frustum, addStaticMeshSurfsForLights);
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

    auto addShadowCasterEntities = [this, view, &viewLight](int32_t proxyId) -> bool {
        DbvtProxy *proxy = (DbvtProxy *)this->dynamicDbvt.GetUserData(proxyId);
        SceneEntity *sceneEntity = proxy->sceneEntity;
            
        // sceneEntity 가 없다면 mesh 이 아님
        if (!sceneEntity) {
            return true;
        }

        if (!sceneEntity->parms.joints) {
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

        for (int surfaceIndex = 0; surfaceIndex < sceneEntity->parms.mesh->NumSurfaces(); surfaceIndex++) {
            MeshSurf *surf = sceneEntity->parms.mesh->GetSurface(surfaceIndex);

            const Material *material = sceneEntity->parms.customMaterials[surf->materialIndex];

            // 이미 ambient visible surf 로 등록되었고, lighting 이 필요한 surf 라면 litSurfs 리스트에 추가한다.
            if (surf->viewCount == this->viewCount && surf->drawSurf->flags & DrawSurf::AmbientVisible && material->IsLitSurface()) {
                drawSurfNode_t *drawSurfNode = (drawSurfNode_t *)frameData.Alloc(sizeof(drawSurfNode_t));
                drawSurfNode->drawSurf = surf->drawSurf;
                drawSurfNode->next = viewLight->litSurfs;
                viewLight->litSurfs = drawSurfNode;
            }
        }

        if (sceneEntity->viewEntity && sceneEntity->viewEntity->ambientVisible) {
            viewLight->litAABB.AddAABB(proxy->aabb);
        }

        if (!sceneEntity->parms.castShadows) {
            return true;
        }

        OBB obb = OBB(sceneEntity->GetAABB(), sceneEntity->parms.origin, sceneEntity->parms.axis);
        if (viewLight->def->CullShadowCasterOBB(obb, view->def->frustum, view->aabb)) {
            return true;
        }

        viewEntity_t *shadowViewEntity = nullptr;
            
        for (int surfaceIndex = 0; surfaceIndex < sceneEntity->parms.mesh->NumSurfaces(); surfaceIndex++) {
            MeshSurf *surf = sceneEntity->parms.mesh->GetSurface(surfaceIndex);

            const Material *material = sceneEntity->parms.customMaterials[surf->materialIndex];

            if (material->IsShadowCaster()) {
                if (surf->viewCount != this->viewCount) {
                    // ambient visible 하지 않으므로 shadow 용 viewEntity 를 등록해준다.
                    if (!shadowViewEntity) {
                        shadowViewEntity = this->AddViewEntity(view, sceneEntity);
                        shadowViewEntity->shadowVisible = true;
                    }

                    if (shadowViewEntity->def->parms.skeleton && shadowViewEntity->def->parms.joints) {
                        shadowViewEntity->def->parms.mesh->UpdateSkinningJointCache(shadowViewEntity->def->parms.skeleton, shadowViewEntity->def->parms.joints);
                    }

                    // drawSurf for shadow
                    this->AddDrawSurf(view, shadowViewEntity, material, surf->subMesh, nullptr, 0);

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
            dynamicDbvt.Query(sceneLight->obb, addShadowCasterEntities);
            break;
        case SceneLight::PointLight:
            if (sceneLight->IsRadiusUniform()) {
                dynamicDbvt.Query(Sphere(sceneLight->GetOrigin(), sceneLight->GetRadius()[0]), addShadowCasterEntities);
            } else {
                dynamicDbvt.Query(sceneLight->obb, addShadowCasterEntities);
            }
            break;
        case SceneLight::SpotLight:
            dynamicDbvt.Query(sceneLight->frustum, addShadowCasterEntities);
            break;
        default:
            break;
        }

        viewLight = viewLight->next;
    }
}

void RenderWorld::OptimizeLights(view_t *view) {
    Rect screenClipRect;
    viewLight_t *prevViewLight = nullptr;

    for (viewLight_t *viewLight = view->viewLights; viewLight; viewLight = viewLight->next) {
        viewLight->litAABB.IntersectSelf(viewLight->def->GetAABB());

        if (!viewLight->def->parms.isMainLight) {
            if (!view->def->GetClipRectFromAABB(viewLight->litAABB, screenClipRect)) {
                if (prevViewLight) {
                    prevViewLight->next = viewLight->next;
                }
                continue;
            }

            Rect visScissorRect = screenClipRect;
            visScissorRect.y = view->def->parms.renderRect.h - (screenClipRect.y + screenClipRect.h);
            viewLight->scissorRect.IntersectSelf(visScissorRect);

            if (viewLight->scissorRect.IsEmpty()) {
                if (prevViewLight) {
                    prevViewLight->next = viewLight->next;
                }
                continue;
            }
        }
    }
}

static int BE_CDECL _CompareDrawSurf(const void *elem1, const void *elem2) {
    const uint32_t sortkey1 = (*(DrawSurf **)elem1)->sortkey;
    const uint32_t sortkey2 = (*(DrawSurf **)elem2)->sortkey;

    if (sortkey1 < sortkey2) {
        return -1;
    }
    if (sortkey1 > sortkey2) {
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
    AddViewLightsAndEntities(view);

    // viewEntities 를 iteration 하며 skinned mesh surface 를 drawSurf 에 등록
    AddSkinnedMeshes(view);

    AddTextMeshes(view);

    // staticDBVT 를 query 해서 찾은 static mesh surface 를 drawSurf 에 등록
    AddStaticMeshes(view);

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

void RenderWorld::AddDrawSurf(view_t *view, viewEntity_t *viewEntity, const Material *material, SubMesh *subMesh, GuiSubMesh *guiSubMesh, int flags) {
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

    if (subMesh) {
        if (viewEntity->def->parms.skeleton && viewEntity->def->parms.joints && !subMesh->IsGpuSkinning()) {
            subMesh->CacheDynamicDataToGpu(viewEntity->def->parms.joints, realMaterial);
        } else {
            subMesh->CacheStaticDataToGpu();
        }
    }

    DrawSurf *drawSurf = (DrawSurf *)frameData.ClearedAlloc(sizeof(DrawSurf));
    drawSurf->entity            = viewEntity;
    drawSurf->material          = realMaterial;
    drawSurf->materialRegisters = nullptr;//outputValues;
    drawSurf->subMesh           = subMesh;
    drawSurf->guiSubMesh        = guiSubMesh;
    drawSurf->flags             = flags;

    drawSurf->MakeSortKey(viewEntity->def->index, realMaterial);
    
    view->drawSurfs[view->numDrawSurfs++] = drawSurf;
}

BE_NAMESPACE_END
