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

    renderWorldGroupId = BE1::Engine::taskManager->CreateGroupId();
}

void RenderWorld::ClearScene() {
#ifdef USE_DBVT
    objectDbvt.Clear();
    staticMeshDbvt.Clear();
    pendingStaticMeshProxyObjects.Clear();
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

#ifdef USE_DBVT
void RenderWorld::CreateObjectProxy(RenderObject *renderObject) {
    if (!renderObject) {
        return;
    }

    // Add proxy node in the DBVT for the renderObjects
    renderObject->proxy = (DbvtProxy *)Mem_Alloc(sizeof(DbvtProxy));
    renderObject->proxy->renderObject = renderObject;
    renderObject->proxy->staticMeshInstance = nullptr;
    renderObject->proxy->worldAABB = renderObject->GetWorldAABB();
    renderObject->proxy->staticMeshSurfIndex = -1;
    renderObject->proxy->id = objectDbvt.CreateProxy(renderObject->proxy->worldAABB, BE1::MeterToUnit(0.0f), renderObject->proxy);
}

void RenderWorld::UpdateObjectProxy(RenderObject *renderObject, const BE1::Mat3x4 &worldMatrix, const BE1::Vec3 &displacementVector) {
    if (!renderObject || !renderObject->proxy) {
        return;
    }

    const BE1::AABB *localAABB = &renderObject->desc.aabb;
    if (localAABB->IsCleared() && renderObject->desc.meshInstance && renderObject->desc.meshInstance->IsSourceReady()) {
        localAABB = &renderObject->desc.meshInstance->GetAABB();
    }

    BE1::AABB worldAABB = BE1::AABB::empty;
    if (!localAABB->IsCleared()) {
        worldAABB.SetFromTransformedAABBFast(*localAABB, worldMatrix);
    }

    renderObject->worldAABB = worldAABB;
    if (renderObject->proxy->worldAABB == worldAABB) {
        return;
    }

    renderObject->proxy->worldAABB = worldAABB;
    objectDbvt.MoveProxy(renderObject->proxy->id, renderObject->proxy->worldAABB, BE1::MeterToUnit(0.5f), displacementVector);
}

void RenderWorld::RegisterPendingStaticMeshProxy(RenderObject *renderObject) {
    if (!renderObject || renderObject->meshSurfProxyPending) {
        return;
    }

    pendingStaticMeshProxyObjects.Append(renderObject->index);
    renderObject->meshSurfProxyPending = true;
}

void RenderWorld::UnregisterPendingStaticMeshProxy(RenderObject *renderObject) {
    if (!renderObject || !renderObject->meshSurfProxyPending) {
        return;
    }

    pendingStaticMeshProxyObjects.RemoveFast(renderObject->index);
    renderObject->meshSurfProxyPending = false;
}

void RenderWorld::DestroyStaticMeshSurfProxies(RenderObject *renderObject) {
    if (!renderObject) {
        return;
    }

    if (renderObject->meshSurfProxies) {
        for (int surfaceIndex = 0; surfaceIndex < renderObject->numMeshSurfProxies; surfaceIndex++) {
            staticMeshDbvt.DestroyProxy(renderObject->meshSurfProxies[surfaceIndex].id);
        }
        Mem_Free(renderObject->meshSurfProxies);
    }

    renderObject->meshSurfProxies = nullptr;
    renderObject->numMeshSurfProxies = 0;
    renderObject->meshSurfProxyDataSerial = ~0u;
}

bool RenderWorld::TryUpdateStaticMeshSurfProxies(RenderObject *renderObject, const BE1::Mat3x4 &worldMatrix, const BE1::Vec3 &displacementVector, bool forceRebuild, bool proxyMoved) {
    if (!renderObject) {
        return true;
    }

    const std::shared_ptr<MeshInstance> &meshInstancePtr = renderObject->desc.meshInstance;
    if (!meshInstancePtr || !meshInstancePtr->IsStatic()) {
        DestroyStaticMeshSurfProxies(renderObject);
        UnregisterPendingStaticMeshProxy(renderObject);
        return true;
    }

    const std::shared_ptr<Mesh> &sourceMesh = meshInstancePtr->GetSourceMesh();
    if (!sourceMesh || sourceMesh->GetState() == Mesh::State::Failed) {
        DestroyStaticMeshSurfProxies(renderObject);
        UnregisterPendingStaticMeshProxy(renderObject);
        return true;
    }

    if (sourceMesh->GetState() != Mesh::State::Ready || !meshInstancePtr->EnsureCurrent()) {
        DestroyStaticMeshSurfProxies(renderObject);
        RegisterPendingStaticMeshProxy(renderObject);
        return false;
    }

    UpdateObjectProxy(renderObject, worldMatrix, displacementVector);

    const int numSurfaces = meshInstancePtr->NumSurfaces();
    const uint32_t dataSerial = sourceMesh->GetDataSerial();
    if (numSurfaces <= 0) {
        DestroyStaticMeshSurfProxies(renderObject);
        renderObject->meshSurfProxyDataSerial = dataSerial;
        UnregisterPendingStaticMeshProxy(renderObject);
        return true;
    }

    const bool rebuild =
        forceRebuild ||
        !renderObject->meshSurfProxies ||
        renderObject->numMeshSurfProxies != numSurfaces ||
        renderObject->meshSurfProxyDataSerial != dataSerial;

    if (rebuild) {
        DestroyStaticMeshSurfProxies(renderObject);

        DbvtProxy *meshSurfProxies = (DbvtProxy *)Mem_Alloc(numSurfaces * sizeof(DbvtProxy));
        int createdSurfaces = 0;

        for (int surfaceIndex = 0; surfaceIndex < numSurfaces; surfaceIndex++) {
            const Mesh::Surface *meshSurf = meshInstancePtr->GetSurface(surfaceIndex);
            if (!meshSurf || !meshSurf->subMesh) {
                for (int createdIndex = 0; createdIndex < createdSurfaces; createdIndex++) {
                    staticMeshDbvt.DestroyProxy(meshSurfProxies[createdIndex].id);
                }
                Mem_Free(meshSurfProxies);
                RegisterPendingStaticMeshProxy(renderObject);
                return false;
            }

            DbvtProxy *meshSurfProxy = &meshSurfProxies[surfaceIndex];
            meshSurfProxy->renderObject = renderObject;
            meshSurfProxy->staticMeshInstance = meshInstancePtr.get();
            meshSurfProxy->worldAABB.SetFromTransformedAABBFast(meshSurf->subMesh->GetAABB(), worldMatrix);
            meshSurfProxy->staticMeshSurfIndex = surfaceIndex;
            meshSurfProxy->id = staticMeshDbvt.CreateProxy(meshSurfProxy->worldAABB, BE1::MeterToUnit(0.0f), meshSurfProxy);
            createdSurfaces++;
        }

        renderObject->numMeshSurfProxies = numSurfaces;
        renderObject->meshSurfProxies = meshSurfProxies;
        renderObject->meshSurfProxyDataSerial = dataSerial;
    } else if (proxyMoved) {
        for (int surfaceIndex = 0; surfaceIndex < numSurfaces; surfaceIndex++) {
            const Mesh::Surface *meshSurf = meshInstancePtr->GetSurface(surfaceIndex);
            if (!meshSurf || !meshSurf->subMesh) {
                DestroyStaticMeshSurfProxies(renderObject);
                RegisterPendingStaticMeshProxy(renderObject);
                return false;
            }

            renderObject->meshSurfProxies[surfaceIndex].worldAABB.SetFromTransformedAABBFast(meshSurf->subMesh->GetAABB(), worldMatrix);
            staticMeshDbvt.MoveProxy(renderObject->meshSurfProxies[surfaceIndex].id, renderObject->meshSurfProxies[surfaceIndex].worldAABB, BE1::MeterToUnit(0.5f), displacementVector);
        }
    }

    UnregisterPendingStaticMeshProxy(renderObject);
    return true;
}

void RenderWorld::UpdatePendingStaticMeshSurfProxies() {
    BE1::Array<int> pendingObjects;
    pendingObjects.Swap(pendingStaticMeshProxyObjects);

    for (int objectIndex : pendingObjects) {
        if (!renderObjects.IsValidIndex(objectIndex)) {
            continue;
        }

        RenderObject *renderObject = renderObjects[objectIndex];
        if (!renderObject) {
            continue;
        }

        renderObject->meshSurfProxyPending = false;
        TryUpdateStaticMeshSurfProxies(renderObject, renderObject->desc.worldMatrix, BE1::Vec3::zero, false, false);
    }
}
#endif

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
        CreateObjectProxy(renderObject);

        TryUpdateStaticMeshSurfProxies(renderObject, renderObject->desc.worldMatrix, BE1::Vec3::zero, true, false);
#endif
    } else {
#ifdef USE_DBVT
        const bool transformChanged = BE1::HasFlag(updateFlags, RenderObject::Desc::UpdateFlag::Transform);
        const bool aabbChanged = BE1::HasFlag(updateFlags, RenderObject::Desc::UpdateFlag::AABB);
        const bool meshChanged = BE1::HasFlag(updateFlags, RenderObject::Desc::UpdateFlag::Mesh);

        BE1::Vec3 displacementVector = BE1::Vec3::zero;
        if (transformChanged) {
            const BE1::Mat3x4 &newWorldMatrix = desc.worldMatrix;
            displacementVector = newWorldMatrix.ToTranslationVec3() - renderObject->desc.worldMatrix.ToTranslationVec3();
        }
#endif
        renderObject->Update(desc, updateFlags);
#ifdef USE_DBVT
        if (transformChanged || aabbChanged || meshChanged) {
            UpdateObjectProxy(renderObject, renderObject->desc.worldMatrix, displacementVector);
        }
        if (transformChanged || meshChanged || renderObject->meshSurfProxyPending) {
            TryUpdateStaticMeshSurfProxies(renderObject, renderObject->desc.worldMatrix, displacementVector, meshChanged, transformChanged);
        }
#endif
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
    UnregisterPendingStaticMeshProxy(renderObject);
    objectDbvt.DestroyProxy(renderObject->proxy->id);
    DestroyStaticMeshSurfProxies(renderObject);
#endif

    delete renderObjects[index];
    renderObjects[index] = nullptr;
}
