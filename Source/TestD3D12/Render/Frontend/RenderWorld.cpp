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
#include "Render/RenderSystem.h"
#include "Render/Mesh/MeshInstance.h"

RenderWorld::RenderWorld() {
    renderObjects.Reserve(65536);
    renderObjects.SetGranularity(4096);

    textMesh.SetCoordFrame(GuiMesh::CoordFrame::CoordFrame3D);

    debugLineColor.Set(0, 0, 0, 0);
    debugFillColor.Set(0, 0, 0, 0);

    renderWorldGroupId = BE1::Engine::taskManager->CreateGroupId();
}

RenderWorld::~RenderWorld() {
    if (renderSystem) {
        renderSystem->GetProbeBakeSystem().CancelEnvironmentLightProbeBake(this);
    }
}

void RenderWorld::BeginFrame() {
    assert(BE1::Engine::IsInMainThread());
    assert(!frameActive);

    frameActive = true;
    frameIndex++;

    ExpireDebugPrimitives(BE1::PlatformTime::Microseconds());
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

    if (BE1::HasFlag(clearFlags, ClearFlag::EnvironmentLight) && hasEnvironmentLight) {
        renderSystem->GetProbeBakeSystem().CancelEnvironmentLightProbeBake(this);
        environmentLight.Reset();
        hasEnvironmentLight = false;
    }

    if (BE1::HasFlag(clearFlags, ClearFlag::OceanSurface) && hasOceanSurface) {
        oceanSurface = OceanSurface();
        hasOceanSurface = false;
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

RenderObject *RenderWorld::GetRenderObjectForUpdate(int index, const char *functionName) const {
    assert(BE1::Engine::IsInMainThread());
    assert(!frameActive);

    if (!renderObjects.IsValidIndex(index)) {
        BE_WARNLOG("%s: invalid index %i\n", functionName, index);
        return nullptr;
    }

    RenderObject *renderObject = renderObjects[index];
    if (!renderObject) {
        BE_WARNLOG("%s: index %i is nullptr\n", functionName, index);
        return nullptr;
    }

    return renderObject;
}

void RenderWorld::UpdateRenderObject(int index, const RenderObject::Desc &desc, RenderObject::Desc::UpdateFlag updateFlags) {
    RenderObject *renderObject = GetRenderObjectForUpdate(index, "RenderWorld::UpdateRenderObject");
    if (!renderObject) {
        return;
    }

    const bool transformChanged = BE1::HasFlag(updateFlags, RenderObject::Desc::UpdateFlag::Transform);
    if (transformChanged) {
        CapturePrevWorldMatrix(renderObject);
    }

    renderObject->Update(desc, updateFlags);
}

void RenderWorld::UpdateRenderObjectFlags(int index, RenderObject::Flag flags) {
    RenderObject *renderObject = GetRenderObjectForUpdate(index, "RenderWorld::UpdateRenderObjectFlags");
    if (renderObject) {
        renderObject->UpdateFlags(flags);
    }
}

void RenderWorld::UpdateRenderObjectLayer(int index, uint8_t layer) {
    RenderObject *renderObject = GetRenderObjectForUpdate(index, "RenderWorld::UpdateRenderObjectLayer");
    if (renderObject) {
        renderObject->UpdateLayer(layer);
    }
}

void RenderWorld::UpdateRenderObjectTransform(int index, const BE1::Mat3x4 &worldMatrix) {
    RenderObject *renderObject = GetRenderObjectForUpdate(index, "RenderWorld::UpdateRenderObjectTransform");
    if (!renderObject) {
        return;
    }

    CapturePrevWorldMatrix(renderObject);
    renderObject->UpdateTransform(worldMatrix);
}

void RenderWorld::UpdateRenderObjectBounds(int index, const BE1::AABB &aabb) {
    RenderObject *renderObject = GetRenderObjectForUpdate(index, "RenderWorld::UpdateRenderObjectBounds");
    if (renderObject) {
        renderObject->UpdateBounds(aabb);
    }
}

void RenderWorld::UpdateRenderObjectMesh(int index, const SharedPtr<MeshInstance> &meshInstance) {
    RenderObject *renderObject = GetRenderObjectForUpdate(index, "RenderWorld::UpdateRenderObjectMesh");
    if (renderObject) {
        renderObject->UpdateMesh(meshInstance);
    }
}

void RenderWorld::UpdateRenderObjectSkeleton(int index, const SharedPtr<Skeleton> &skeleton) {
    RenderObject *renderObject = GetRenderObjectForUpdate(index, "RenderWorld::UpdateRenderObjectSkeleton");
    if (renderObject) {
        renderObject->UpdateSkeleton(skeleton);
    }
}

void RenderWorld::UpdateRenderObjectMeshLOD(int index, int lodLevel) {
    RenderObject *renderObject = GetRenderObjectForUpdate(index, "RenderWorld::UpdateRenderObjectMeshLOD");
    if (renderObject) {
        renderObject->UpdateMeshLOD(lodLevel);
    }
}

void RenderWorld::UpdateRenderObjectMaterials(int index, const BE1::Array<SharedPtr<Material>> &materials) {
    RenderObject *renderObject = GetRenderObjectForUpdate(index, "RenderWorld::UpdateRenderObjectMaterials");
    if (renderObject) {
        renderObject->UpdateMaterials(materials);
    }
}

void RenderWorld::UpdateRenderObjectFont(int index, const SharedPtr<Font> &font) {
    RenderObject *renderObject = GetRenderObjectForUpdate(index, "RenderWorld::UpdateRenderObjectFont");
    if (renderObject) {
        renderObject->UpdateFont(font);
    }
}

void RenderWorld::UpdateRenderObjectText(int index, const BE1::Str &text) {
    RenderObject *renderObject = GetRenderObjectForUpdate(index, "RenderWorld::UpdateRenderObjectText");
    if (renderObject) {
        renderObject->UpdateText(text);
    }
}

void RenderWorld::UpdateRenderObjectTextStyle(int index, const RenderObject::TextDesc &textDesc) {
    RenderObject *renderObject = GetRenderObjectForUpdate(index, "RenderWorld::UpdateRenderObjectTextStyle");
    if (renderObject) {
        renderObject->UpdateTextStyle(textDesc);
    }
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

static bool HasDirectionalLight(const BE1::Array<RenderLight *> &renderLights, const RenderLight *excludedLight = nullptr) {
    for (const RenderLight *renderLight : renderLights) {
        if (renderLight && renderLight != excludedLight && renderLight->GetType() == RenderLight::Type::Directional) {
            return true;
        }
    }
    return false;
}

int RenderWorld::AddRenderLight(const RenderLight::Desc &desc) {
    assert(BE1::Engine::IsInMainThread());
    assert(!frameActive);

    if (desc.type == RenderLight::Type::Directional && HasDirectionalLight(renderLights)) {
        BE_WARNLOG("RenderWorld::AddRenderLight: only one directional light is supported\n");
        return -1;
    }

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

RenderLight *RenderWorld::GetRenderLightForUpdate(int index, const char *functionName) const {
    assert(BE1::Engine::IsInMainThread());
    assert(!frameActive);

    if (!renderLights.IsValidIndex(index)) {
        BE_WARNLOG("%s: invalid index %i\n", functionName, index);
        return nullptr;
    }

    RenderLight *renderLight = renderLights[index];
    if (!renderLight) {
        BE_WARNLOG("%s: index %i is nullptr\n", functionName, index);
        return nullptr;
    }

    return renderLight;
}

void RenderWorld::UpdateRenderLight(int index, const RenderLight::Desc &desc, RenderLight::Desc::UpdateFlag updateFlags) {
    RenderLight *renderLight = GetRenderLightForUpdate(index, "RenderWorld::UpdateRenderLight");
    if (!renderLight) {
        return;
    }

    // 이미 Directional Light 가 존재하는데 다른 Light 를 Directional 로 바꿀 수는 없다.
    if (BE1::HasFlag(updateFlags, RenderLight::Desc::UpdateFlag::Shape) &&
        desc.type == RenderLight::Type::Directional &&
        renderLight->GetType() != RenderLight::Type::Directional &&
        HasDirectionalLight(renderLights, renderLight)) {
        BE_WARNLOG("RenderWorld::UpdateRenderLight: only one directional light is supported\n");
        return;
    }

    renderLight->Update(desc, updateFlags);
}

void RenderWorld::UpdateRenderLightFlags(int index, RenderLight::Flag flags) {
    RenderLight *renderLight = GetRenderLightForUpdate(index, "RenderWorld::UpdateRenderLightFlags");
    if (renderLight) {
        renderLight->UpdateFlags(flags);
    }
}

void RenderWorld::UpdateRenderLightLayer(int index, uint8_t layer) {
    RenderLight *renderLight = GetRenderLightForUpdate(index, "RenderWorld::UpdateRenderLightLayer");
    if (renderLight) {
        renderLight->UpdateLayer(layer);
    }
}

void RenderWorld::UpdateRenderLightTransform(int index, const BE1::Vec3 &origin, const BE1::Mat3 &axis) {
    RenderLight *renderLight = GetRenderLightForUpdate(index, "RenderWorld::UpdateRenderLightTransform");
    if (renderLight) {
        renderLight->UpdateTransform(origin, axis);
    }
}

void RenderWorld::UpdateRenderLightShape(int index, RenderLight::Type type, const BE1::Vec3 &size, float zNear) {
    RenderLight *renderLight = GetRenderLightForUpdate(index, "RenderWorld::UpdateRenderLightShape");
    if (!renderLight) {
        return;
    }

    // 이미 Directional Light 가 존재하는데 다른 Light 를 Directional 로 바꿀 수는 없다.
    if (type == RenderLight::Type::Directional &&
        renderLight->GetType() != RenderLight::Type::Directional &&
        HasDirectionalLight(renderLights, renderLight)) {
        BE_WARNLOG("RenderWorld::UpdateRenderLightShape: only one directional light is supported\n");
        return;
    }

    renderLight->UpdateShape(type, size, zNear);
}

void RenderWorld::UpdateRenderLightProjectionPolicy(int index, const RHI::ProjectionPolicy &projectionPolicy) {
    RenderLight *renderLight = GetRenderLightForUpdate(index, "RenderWorld::UpdateRenderLightProjectionPolicy");
    if (renderLight) {
        renderLight->UpdateProjectionPolicy(projectionPolicy);
    }
}

void RenderWorld::UpdateRenderLightLighting(int index, const BE1::Color3 &linearColor, float intensity, float fallOffExponent) {
    RenderLight *renderLight = GetRenderLightForUpdate(index, "RenderWorld::UpdateRenderLightLighting");
    if (renderLight) {
        renderLight->UpdateLighting(linearColor, intensity, fallOffExponent);
    }
}

void RenderWorld::UpdateRenderLightShadow(int index, float shadowOffsetFactor, float shadowOffsetUnits) {
    RenderLight *renderLight = GetRenderLightForUpdate(index, "RenderWorld::UpdateRenderLightShadow");
    if (renderLight) {
        renderLight->UpdateShadow(shadowOffsetFactor, shadowOffsetUnits);
    }
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

bool RenderWorld::AddEnvironmentLight(const EnvironmentLight::Desc &desc) {
    assert(BE1::Engine::IsInMainThread());
    assert(!frameActive);

    if (hasEnvironmentLight) {
        BE_WARNLOG("RenderWorld::AddEnvironmentLight: only one environment light is supported\n");
        return false;
    }

    environmentLight.Update(desc);
    hasEnvironmentLight = true;
    return true;
}

void RenderWorld::UpdateEnvironmentLight(const EnvironmentLight::Desc &desc) {
    assert(BE1::Engine::IsInMainThread());
    assert(!frameActive);

    if (!hasEnvironmentLight) {
        BE_WARNLOG("RenderWorld::UpdateEnvironmentLight: no environment light has been added\n");
        return;
    }

    const bool radianceCubeMapChanged = environmentLight.GetDesc().radianceCubeMap != desc.radianceCubeMap;
    environmentLight.Update(desc);

    if (radianceCubeMapChanged) {
        renderSystem->GetProbeBakeSystem().CancelEnvironmentLightProbeBake(this);
    }
}

bool RenderWorld::SetEnvironmentLightBakedCubeMaps(const SharedPtr<Texture> &diffuseIrradianceCubeMap, const SharedPtr<Texture> &specularPrefilteredCubeMap) {
    assert(BE1::Engine::IsInMainThread());
    assert(!frameActive);

    if (!hasEnvironmentLight) {
        BE_WARNLOG("RenderWorld::SetEnvironmentLightBakedCubeMaps: no environment light has been added\n");
        return false;
    }

    environmentLight.SetBakedCubeMaps(diffuseIrradianceCubeMap, specularPrefilteredCubeMap);
    renderSystem->GetProbeBakeSystem().CancelEnvironmentLightProbeBake(this);
    return true;
}

void RenderWorld::RequestEnvironmentLightCubemapRegeneration(const BE1::Vec3 &captureOrigin) {
    assert(BE1::Engine::IsInMainThread());
    assert(!frameActive);

    if (!hasEnvironmentLight) {
        BE_WARNLOG("RenderWorld::RequestEnvironmentLightCubemapRegeneration: no environment light has been added\n");
        return;
    }

    environmentLight.MarkRegenerationNeeded();
    renderSystem->GetProbeBakeSystem().ScheduleEnvironmentLightProbeBake(this, captureOrigin);
}

void RenderWorld::RemoveEnvironmentLight() {
    assert(BE1::Engine::IsInMainThread());
    assert(!frameActive);

    if (!hasEnvironmentLight) {
        BE_WARNLOG("RenderWorld::RemoveEnvironmentLight: no environment light has been added\n");
        return;
    }

    renderSystem->GetProbeBakeSystem().CancelEnvironmentLightProbeBake(this);
    environmentLight.Reset();
    hasEnvironmentLight = false;
}

bool RenderWorld::AddOceanSurface(const OceanSurface::Desc &desc) {
    assert(BE1::Engine::IsInMainThread());
    assert(!frameActive);

    if (hasOceanSurface) {
        BE_WARNLOG("RenderWorld::AddOceanSurface: only one ocean surface is supported\n");
        return false;
    }

    if (!desc.IsValid()) {
        BE_WARNLOG("RenderWorld::AddOceanSurface: invalid description\n");
        return false;
    }

    oceanSurface.Update(desc);
    hasOceanSurface = true;
    return true;
}

bool RenderWorld::UpdateOceanSurface(const OceanSurface::Desc &desc) {
    assert(BE1::Engine::IsInMainThread());
    assert(!frameActive);

    if (!hasOceanSurface) {
        BE_WARNLOG("RenderWorld::UpdateOceanSurface: no ocean surface has been added\n");
        return false;
    }

    if (!desc.IsValid()) {
        BE_WARNLOG("RenderWorld::UpdateOceanSurface: invalid description\n");
        return false;
    }

    oceanSurface.Update(desc);
    return true;
}

void RenderWorld::RemoveOceanSurface() {
    assert(BE1::Engine::IsInMainThread());
    assert(!frameActive);

    if (!hasOceanSurface) {
        BE_WARNLOG("RenderWorld::RemoveOceanSurface: no ocean surface has been added\n");
        return;
    }

    oceanSurface = OceanSurface();
    hasOceanSurface = false;
}

bool RenderWorld::SetOceanSurfaceQueries(const OceanSurfaceQueryInput *queries, uint32_t queryCount) {
    assert(BE1::Engine::IsInMainThread());
    assert(!frameActive);

    if (!hasOceanSurface) {
        BE_WARNLOG("RenderWorld::SetOceanSurfaceQueries: no ocean surface has been added\n");
        return false;
    }

    if ((queryCount > 0 && !queries) || queryCount > GpuOceanSurface::MaxQueryCount) {
        BE_WARNLOG("RenderWorld::SetOceanSurfaceQueries: invalid query data or count %u\n", queryCount);
        return false;
    }

    oceanSurface.queries.SetCount(static_cast<int>(queryCount), false);
    if (queryCount > 0) {
        BE1::simdProcessor->Memcpy(oceanSurface.queries.Ptr(), queries, static_cast<int>(queryCount * sizeof(OceanSurfaceQueryInput)));
    }
    return true;
}

bool RenderWorld::GetSurfaceHeightRange(float &minimumHeight, float &maximumHeight) const {
    if (!hasOceanSurface) {
        return false;
    }

    const GpuOceanSurface::Settings &surfaceSettings = oceanSurface.GetDesc().surface;
    minimumHeight = surfaceSettings.height - surfaceSettings.maxDisplacement;
    maximumHeight = surfaceSettings.height + surfaceSettings.maxDisplacement;
    return true;
}

bool RenderWorld::SubmitQueries(const BE1::PhysBuoyancySurfaceQuery *queries, uint32_t queryCount) {
    assert(BE1::Engine::IsInMainThread());
    assert(!frameActive);

    if (!hasOceanSurface || (queryCount > 0 && !queries) || queryCount > GpuOceanSurface::MaxQueryCount) {
        return false;
    }

    oceanSurface.queries.SetCount(static_cast<int>(queryCount), false);

    for (uint32_t queryIndex = 0; queryIndex < queryCount; ++queryIndex) {
        const BE1::PhysBuoyancySurfaceQuery &sourceQuery = queries[queryIndex];
        OceanSurfaceQueryInput &destinationQuery = oceanSurface.queries[queryIndex];
        destinationQuery.worldPositionXY = float2(sourceQuery.worldPosition.x, sourceQuery.worldPosition.y);
        destinationQuery.sampleDiameter = sourceQuery.sampleDiameter;
        destinationQuery.queryId = sourceQuery.queryId;
    }
    return true;
}

bool RenderWorld::GetQueryResult(uint32_t queryId, BE1::PhysBuoyancySurfaceQueryResult &result) const {
    if (!hasOceanSurface || !renderSystem) {
        return false;
    }

    OceanSurfaceQueryResult oceanResult;
    if (!renderSystem->GetGpuOceanSurface().GetQueryResult(queryId, oceanResult)) {
        return false;
    }

    result.surfacePosition.Set(oceanResult.surfacePosition.x, oceanResult.surfacePosition.y, oceanResult.surfacePosition.z);
    result.surfaceNormal.Set(oceanResult.surfaceNormal.x, oceanResult.surfaceNormal.y, oceanResult.surfaceNormal.z);
    return true;
}

void RenderWorld::CapturePrevWorldMatrix(RenderObject *renderObject) {
    const uint64_t previousFrameIndex = frameIndex;
    if (previousFrameIndex == 0) {
        return;
    }

    // 직전 프레임에 렌더링되지 않은 오브젝트는 이전 월드 매트릭스를 캡처하지 않는다.
    if (renderObject->visibleFrameIndex != previousFrameIndex) {
        return;
    }

    if (renderObject->prevWorldMatrixFrameIndex == previousFrameIndex) {
        return;
    }

    renderObject->prevWorldMatrix = renderObject->desc.worldMatrix;
    renderObject->prevWorldMatrixFrameIndex = previousFrameIndex;
}
