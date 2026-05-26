// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Precompiled.h"
#include "RenderWorld.h"
#include "MeshInstance.h"

RenderWorld::RenderWorld() {
    renderObjects.Reserve(16384);
    renderObjects.SetGranularity(4096);

    textMesh.SetCoordFrame(GuiMesh::CoordFrame::CoordFrame3D);

    debugLineColor.Set(0, 0, 0, 0);
    debugFillColor.Set(0, 0, 0, 0);
}

void RenderWorld::ClearScene() {
#ifdef USE_DBVT
    objectDbvt.Clear();
    staticMeshDbvt.Clear();
#endif

    for (RenderObject *renderObject : renderObjects) {
        SAFE_DELETE(renderObject);
    }
}

RenderObject *RenderWorld::GetRenderObject(int index) const {
    if (index < 0 || index >= renderObjects.Count()) {
        BE_WARNLOG("RenderWorld::GetRenderObject: index %i > %i\n", index, renderObjects.Count() - 1);
        return nullptr;
    }

    RenderObject *renderObject = renderObjects[index];
    if (!renderObject) {
        BE_WARNLOG("RenderWorld::GetRenderObject: index %i is nullptr\n", index);
        return nullptr;
    }

    return renderObject;
}

int RenderWorld::AddRenderObject(const RenderObject::Desc &desc) {
    assert(BE1::Engine::IsInMainThread());

    int index = renderObjects.FindNull();
    if (index == -1) {
        index = renderObjects.Append(nullptr);
    }

    UpdateRenderObject(index, desc, RenderObject::Desc::UpdateFlag::All);
    return index;
}

void RenderWorld::UpdateRenderObject(int index, const RenderObject::Desc &desc, RenderObject::Desc::UpdateFlag updateFlags) {
    RenderObject *renderObject = renderObjects[index];
    if (!renderObject) {
        renderObject = new RenderObject;
        renderObject->index = index;
        renderObjects[index] = renderObject;
        renderObject->Update(desc, updateFlags);

#ifdef USE_DBVT
        // Add proxy node in the DBVT for the renderObjects
        renderObject->proxy = (DbvtProxy *)Mem_Alloc(sizeof(DbvtProxy));
        renderObject->proxy->renderObject = renderObject;
        renderObject->proxy->staticMeshInstance = nullptr;
        renderObject->proxy->worldAABB = renderObject->GetWorldAABB();
        renderObject->proxy->staticMeshSurfIndex = -1;
        renderObject->proxy->id = objectDbvt.CreateProxy(renderObject->proxy->worldAABB, BE1::MeterToUnit(0.0f), renderObject->proxy);

        if (desc.meshInstance && desc.meshInstance->IsStatic()) {
            if (desc.meshInstance->NumSurfaces() > 0) {
                renderObject->numMeshSurfProxies = desc.meshInstance->NumSurfaces();
                renderObject->meshSurfProxies = (DbvtProxy *)Mem_Alloc(desc.meshInstance->NumSurfaces() * sizeof(DbvtProxy));

                for (int surfaceIndex = 0; surfaceIndex < desc.meshInstance->NumSurfaces(); ++surfaceIndex) {
                    const Mesh::Surface *meshSurf = desc.meshInstance->GetSurface(surfaceIndex);

                    DbvtProxy *meshSurfProxy = &renderObject->meshSurfProxies[surfaceIndex];
                    meshSurfProxy->renderObject = renderObject;
                    meshSurfProxy->staticMeshInstance = desc.meshInstance.get();
                    meshSurfProxy->worldAABB.SetFromTransformedAABBFast(meshSurf->subMesh->GetAABB(), desc.worldMatrix);
                    meshSurfProxy->staticMeshSurfIndex = surfaceIndex;
                    meshSurfProxy->id = staticMeshDbvt.CreateProxy(renderObject->meshSurfProxies[surfaceIndex].worldAABB, BE1::MeterToUnit(0.0f), &renderObject->meshSurfProxies[surfaceIndex]);
                }
            }
        }
#endif
    } else {
#ifdef USE_DBVT
        const bool transformChanged = BE1::HasFlag(updateFlags, RenderObject::Desc::UpdateFlag::Transform);
        const bool aabbChanged = BE1::HasFlag(updateFlags, RenderObject::Desc::UpdateFlag::AABB);
        const bool meshChanged = BE1::HasFlag(updateFlags, RenderObject::Desc::UpdateFlag::Mesh);
        const bool proxyMoved = transformChanged || aabbChanged;

        const BE1::Mat3x4 &newWorldMatrix = transformChanged ? desc.worldMatrix : renderObject->desc.worldMatrix;
        const BE1::AABB &newAABB = aabbChanged ? desc.aabb : renderObject->desc.aabb;

        BE1::Vec3 displacementVector = BE1::Vec3::zero;
        if (transformChanged) {
            displacementVector = newWorldMatrix.ToTranslationVec3() - renderObject->desc.worldMatrix.ToTranslationVec3();
        }
        if (transformChanged || aabbChanged) {
            renderObject->proxy->worldAABB.SetFromTransformedAABBFast(newAABB, newWorldMatrix);
            objectDbvt.MoveProxy(renderObject->proxy->id, renderObject->proxy->worldAABB, BE1::MeterToUnit(0.5f), displacementVector);
        }

        if (meshChanged) {
            for (int surfaceIndex = 0; surfaceIndex < renderObject->numMeshSurfProxies; surfaceIndex++) {
                staticMeshDbvt.DestroyProxy(renderObject->meshSurfProxies[surfaceIndex].id);
            }
            Mem_Free(renderObject->meshSurfProxies);
            renderObject->meshSurfProxies = nullptr;
            renderObject->numMeshSurfProxies = 0;

            if (desc.meshInstance && desc.meshInstance->IsStatic()) {
                renderObject->numMeshSurfProxies = desc.meshInstance->NumSurfaces();
                renderObject->meshSurfProxies = (DbvtProxy *)Mem_ClearedAlloc(renderObject->numMeshSurfProxies * sizeof(DbvtProxy));

                for (int surfaceIndex = 0; surfaceIndex < desc.meshInstance->NumSurfaces(); surfaceIndex++) {
                    const Mesh::Surface *meshSurf = desc.meshInstance->GetSurface(surfaceIndex);

                    DbvtProxy *meshSurfProxy = &renderObject->meshSurfProxies[surfaceIndex];
                    meshSurfProxy->renderObject = renderObject;
                    meshSurfProxy->staticMeshInstance = desc.meshInstance.get();
                    meshSurfProxy->worldAABB.SetFromTransformedAABBFast(meshSurf->subMesh->GetAABB(), newWorldMatrix);
                    meshSurfProxy->staticMeshSurfIndex = surfaceIndex;
                    meshSurfProxy->id = staticMeshDbvt.CreateProxy(renderObject->meshSurfProxies[surfaceIndex].worldAABB, BE1::MeterToUnit(0.0f), &renderObject->meshSurfProxies[surfaceIndex]);
                }
            }
        } else if (proxyMoved) {
            if (renderObject->desc.meshInstance && renderObject->meshSurfProxies) {
                for (int surfaceIndex = 0; surfaceIndex < renderObject->desc.meshInstance->NumSurfaces(); surfaceIndex++) {
                    renderObject->meshSurfProxies[surfaceIndex].worldAABB.SetFromTransformedAABBFast(renderObject->desc.meshInstance->GetSurface(surfaceIndex)->subMesh->GetAABB(), newWorldMatrix);
                    staticMeshDbvt.MoveProxy(renderObject->meshSurfProxies[surfaceIndex].id, renderObject->meshSurfProxies[surfaceIndex].worldAABB, BE1::MeterToUnit(0.5f), displacementVector);
                }
            }
        }
#endif
        renderObject->Update(desc, updateFlags);
    }
}

void RenderWorld::RemoveRenderObject(int index) {
    assert(BE1::Engine::IsInMainThread());

    if (!renderObjects.IsValidIndex(index)) {
        BE_WARNLOG("RenderWorld::RemoveRenderObject: invalid index %i\n", index);
        return;
    }

    RenderObject *renderObject = renderObjects[index];
    if (!renderObject) {
        BE_WARNLOG("RenderWorld::RemoveRenderObject: index %i is nullptr\n", index);
        return;
    }

#ifdef USE_DBVT
    objectDbvt.DestroyProxy(renderObject->proxy->id);

    for (int i = 0; i < renderObject->numMeshSurfProxies; ++i) {
        staticMeshDbvt.DestroyProxy(renderObject->meshSurfProxies[i].id);
    }
#endif

    delete renderObjects[index];
    renderObjects[index] = nullptr;
}
