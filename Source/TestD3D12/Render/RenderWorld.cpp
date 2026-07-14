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
#include "Render/Mesh/MeshInstance.h"

RenderWorld::RenderWorld() {
    renderObjects.Reserve(65536);
    renderObjects.SetGranularity(4096);

    textMesh.SetCoordFrame(GuiMesh::CoordFrame::CoordFrame3D);

    debugLineColor.Set(0, 0, 0, 0);
    debugFillColor.Set(0, 0, 0, 0);

    renderWorldGroupId = BE1::Engine::taskManager->CreateGroupId();
}

void RenderWorld::BeginFrame() {
    assert(BE1::Engine::IsInMainThread());
    assert(!frameActive);

    frameActive = true;
    renderWorldFrameIndex++;

    //ClearDebugLines();
}

void RenderWorld::EndFrame() {
    assert(BE1::Engine::IsInMainThread());
    assert(frameActive);

    frameActive = false;
}

void RenderWorld::ClearScene(ClearFlag clearFlags) {
    assert(BE1::Engine::IsInMainThread());
    assert(!frameActive);

    if (BE1::HasFlag(clearFlags, ClearFlag::RenderObjects)) {
        for (RenderObject *renderObject : renderObjects) {
            SAFE_DELETE(renderObject);
        }
        renderObjects.Clear();
    }

    if (BE1::HasFlag(clearFlags, ClearFlag::RenderLights)) {
        for (RenderLight *renderLight : renderLights) {
            SAFE_DELETE(renderLight);
        }
        renderLights.Clear();
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
    assert(!frameActive);

    int index = renderObjects.FindNull();
    if (index == -1) {
        index = renderObjects.Append(nullptr);
    }

    RenderObject *renderObject = new RenderObject;
    renderObject->index = index;
    renderObjects[index] = renderObject;

    renderObject->Update(desc, RenderObject::Desc::UpdateFlag::All);
    return index;
}

void RenderWorld::UpdateRenderObject(int index, const RenderObject::Desc &desc, RenderObject::Desc::UpdateFlag updateFlags) {
    assert(BE1::Engine::IsInMainThread());
    assert(!frameActive);

    if (!renderObjects.IsValidIndex(index)) {
        BE_WARNLOG("RenderWorld::UpdateRenderObject: invalid index %i\n", index);
        return;
    }

    RenderObject *renderObject = renderObjects[index];
    if (!renderObject) {
        BE_WARNLOG("RenderWorld::UpdateRenderObject: index %i is nullptr\n", index);
        return;
    }

    const bool transformChanged = BE1::HasFlag(updateFlags, RenderObject::Desc::UpdateFlag::Transform);
    if (transformChanged) {
        CapturePrevWorldMatrix(renderObject);
    }

    renderObject->Update(desc, updateFlags);
}

void RenderWorld::RemoveRenderObject(int index) {
    assert(BE1::Engine::IsInMainThread());
    assert(!frameActive);

    if (!renderObjects.IsValidIndex(index)) {
        BE_WARNLOG("RenderWorld::RemoveRenderObject: invalid index %i\n", index);
        return;
    }

    RenderObject *renderObject = renderObjects[index];
    if (!renderObject) {
        BE_WARNLOG("RenderWorld::RemoveRenderObject: index %i is nullptr\n", index);
        return;
    }

    delete renderObjects[index];
    renderObjects[index] = nullptr;
}

RenderLight *RenderWorld::GetRenderLight(int index) const {
    if (index < 0 || index >= renderLights.Count()) {
        BE_WARNLOG("RenderWorld::GetRenderLight: index %i > %i\n", index, renderLights.Count() - 1);
        return nullptr;
    }

    RenderLight *renderLight = renderLights[index];
    if (!renderLight) {
        BE_WARNLOG("RenderWorld::GetRenderLight: index %i is nullptr\n", index);
        return nullptr;
    }

    return renderLight;
}

int RenderWorld::AddRenderLight(const RenderLight::Desc &desc) {
    assert(BE1::Engine::IsInMainThread());
    assert(!frameActive);

    int index = renderLights.FindNull();
    if (index == -1) {
        index = renderLights.Append(nullptr);
    }

    RenderLight *renderLight = new RenderLight;
    renderLight->index = index;
    renderLights[index] = renderLight;

    renderLight->Update(desc, RenderLight::Desc::UpdateFlag::All);
    return index;
}

void RenderWorld::UpdateRenderLight(int index, const RenderLight::Desc &desc, RenderLight::Desc::UpdateFlag updateFlags) {
    assert(BE1::Engine::IsInMainThread());
    assert(!frameActive);

    if (!renderLights.IsValidIndex(index)) {
        BE_WARNLOG("RenderWorld::UpdateRenderLight: invalid index %i\n", index);
        return;
    }

    RenderLight *renderLight = renderLights[index];
    if (!renderLight) {
        BE_WARNLOG("RenderWorld::UpdateRenderLight: index %i is nullptr\n", index);
        return;
    }

    renderLight->Update(desc, updateFlags);
}

void RenderWorld::RemoveRenderLight(int index) {
    assert(BE1::Engine::IsInMainThread());
    assert(!frameActive);

    if (!renderLights.IsValidIndex(index)) {
        BE_WARNLOG("RenderWorld::RemoveRenderLight: invalid index %i\n", index);
        return;
    }

    RenderLight *renderLight = renderLights[index];
    if (!renderLight) {
        BE_WARNLOG("RenderWorld::RemoveRenderLight: index %i is nullptr\n", index);
        return;
    }

    delete renderLights[index];
    renderLights[index] = nullptr;
}

void RenderWorld::CapturePrevWorldMatrix(RenderObject *renderObject) {
    const uint64_t previousFrameIndex = renderWorldFrameIndex;
    if (previousFrameIndex == 0) {
        return;
    }

    // 직전 프레임에 렌더링되지 않은 오브젝트는 이전 월드 매트릭스를 캡처하지 않는다.
    if (renderObject->visibleFrameIndex != previousFrameIndex) {
        return;
    }

    if (renderObject->prevWorldMatrixValid && renderObject->prevWorldMatrixFrameIndex == previousFrameIndex) {
        return;
    }

    renderObject->prevWorldMatrix = renderObject->desc.worldMatrix;
    renderObject->prevWorldMatrixValid = true;
    renderObject->prevWorldMatrixFrameIndex = previousFrameIndex;
}
