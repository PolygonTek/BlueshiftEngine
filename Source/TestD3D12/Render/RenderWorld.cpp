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

#ifdef USE_DBVT
static constexpr uint32_t DynamicObjectSleepDelayMsec = 3000;
#endif

RenderWorld::RenderWorld() {
    renderObjects.Reserve(65536);
    renderObjects.SetGranularity(4096);

    textMesh.SetCoordFrame(GuiMesh::CoordFrame::CoordFrame3D);

    debugLineColor.Set(0, 0, 0, 0);
    debugFillColor.Set(0, 0, 0, 0);

    renderWorldGroupId = BE1::Engine::taskManager->CreateGroupId();
}

void RenderWorld::ClearScene() {
#ifdef USE_DBVT
    objectDbvt.Clear();
    pendingObjectProxyObjects.Clear();
    dynamicObjectIndices.Clear();
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

    renderObject->proxy = (DbvtProxy *)Mem_Alloc(sizeof(DbvtProxy));
    renderObject->proxy->renderObject = renderObject;
    renderObject->proxy->worldAABB = renderObject->GetWorldAABB();
    renderObject->proxy->id = -1;

    InsertObjectProxyToDbvt(renderObject);

    const BE1::AABB *localAABB = &renderObject->desc.aabb;
    if (localAABB->IsCleared() && renderObject->desc.meshInstance && !renderObject->desc.meshInstance->IsSourceReady()) {
        const SharedPtr<Mesh> &sourceMesh = renderObject->desc.meshInstance->GetSourceMesh();
        if (!sourceMesh || sourceMesh->GetState() != Mesh::State::Failed) {
            RegisterPendingObjectProxy(renderObject);
        }
    }
}

void RenderWorld::UpdateObjectProxy(RenderObject *renderObject, const BE1::Mat3x4 &worldMatrix, const BE1::Vec3 &displacementVector) {
    if (!renderObject || !renderObject->proxy) {
        return;
    }

    const BE1::AABB *localAABB = &renderObject->desc.aabb;
    if (localAABB->IsCleared() && renderObject->desc.meshInstance) {
        if (renderObject->desc.meshInstance->IsSourceReady()) {
            localAABB = &renderObject->desc.meshInstance->GetAABB();
            UnregisterPendingObjectProxy(renderObject);
        } else {
            const SharedPtr<Mesh> &sourceMesh = renderObject->desc.meshInstance->GetSourceMesh();
            if (sourceMesh && sourceMesh->GetState() == Mesh::State::Failed) {
                UnregisterPendingObjectProxy(renderObject);
            } else {
                RegisterPendingObjectProxy(renderObject);
            }
        }
    } else {
        UnregisterPendingObjectProxy(renderObject);
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
    if (renderObject->objectProxyInDbvt) {
        objectDbvt.MoveProxy(renderObject->proxy->id, renderObject->proxy->worldAABB, BE1::MeterToUnit(0.5f), displacementVector);
    }
}

void RenderWorld::InsertObjectProxyToDbvt(RenderObject *renderObject) {
    if (!renderObject || !renderObject->proxy || renderObject->objectProxyInDbvt) {
        return;
    }

    renderObject->proxy->id = objectDbvt.CreateProxy(renderObject->proxy->worldAABB, BE1::MeterToUnit(0.0f), renderObject->proxy);
    renderObject->objectProxyInDbvt = true;
}

void RenderWorld::RemoveObjectProxyFromDbvt(RenderObject *renderObject) {
    if (!renderObject || !renderObject->proxy || !renderObject->objectProxyInDbvt) {
        return;
    }

    objectDbvt.DestroyProxy(renderObject->proxy->id);
    renderObject->proxy->id = -1;
    renderObject->objectProxyInDbvt = false;
}

void RenderWorld::RegisterDynamicObject(RenderObject *renderObject) {
    if (!renderObject || renderObject->dynamicObjectRegistered) {
        return;
    }

    RemoveObjectProxyFromDbvt(renderObject);
    dynamicObjectIndices.Append(renderObject->index);
    renderObject->dynamicObjectRegistered = true;
}

void RenderWorld::UnregisterDynamicObject(RenderObject *renderObject) {
    if (!renderObject || !renderObject->dynamicObjectRegistered) {
        return;
    }

    dynamicObjectIndices.RemoveFast(renderObject->index);
    renderObject->dynamicObjectRegistered = false;
}

void RenderWorld::RegisterPendingObjectProxy(RenderObject *renderObject) {
    if (!renderObject || renderObject->objectProxyPending) {
        return;
    }

    pendingObjectProxyObjects.Append(renderObject->index);
    renderObject->objectProxyPending = true;
}

void RenderWorld::UnregisterPendingObjectProxy(RenderObject *renderObject) {
    if (!renderObject || !renderObject->objectProxyPending) {
        return;
    }

    pendingObjectProxyObjects.RemoveFast(renderObject->index);
    renderObject->objectProxyPending = false;
}

void RenderWorld::UpdatePendingObjectProxies() {
    BE1::Array<int> pendingObjects;
    pendingObjects.Swap(pendingObjectProxyObjects);

    for (int objectIndex : pendingObjects) {
        if (!renderObjects.IsValidIndex(objectIndex)) {
            continue;
        }

        RenderObject *renderObject = renderObjects[objectIndex];
        if (!renderObject) {
            continue;
        }

        renderObject->objectProxyPending = false;
        UpdateObjectProxy(renderObject, renderObject->desc.worldMatrix, BE1::Vec3::zero);
    }
}

void RenderWorld::UpdateSleepingObjectProxies() {
    const uint32_t currentMsec = BE1::PlatformTime::Milliseconds();

    const int numDynamicObjectIndices = dynamicObjectIndices.Count();
    for (int i = 0; i < numDynamicObjectIndices;) {
        const int objectIndex = dynamicObjectIndices[i];
        if (!renderObjects.IsValidIndex(objectIndex)) {
            dynamicObjectIndices.RemoveIndexFast(i);
            continue;
        }

        RenderObject *renderObject = renderObjects[objectIndex];
        if (!renderObject) {
            dynamicObjectIndices.RemoveIndexFast(i);
            continue;
        }

        if (currentMsec - renderObject->lastTransformUpdateMsec < DynamicObjectSleepDelayMsec) {
            i++;
            continue;
        }

        renderObject->dynamicObjectRegistered = false;
        dynamicObjectIndices.RemoveIndexFast(i);
        InsertObjectProxyToDbvt(renderObject);
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
            if (transformChanged) {
                renderObject->lastTransformUpdateMsec = BE1::PlatformTime::Milliseconds();
                RegisterDynamicObject(renderObject);
            }
            UpdateObjectProxy(renderObject, renderObject->desc.worldMatrix, displacementVector);
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
    UnregisterPendingObjectProxy(renderObject);
    UnregisterDynamicObject(renderObject);
    RemoveObjectProxyFromDbvt(renderObject);
#endif

    delete renderObjects[index];
    renderObjects[index] = nullptr;
}
