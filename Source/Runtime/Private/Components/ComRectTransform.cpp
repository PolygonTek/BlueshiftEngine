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
#include "Components/ComRectTransform.h"
#include "Components/ComCanvas.h"
#include "Game/Entity.h"
#include "Game/GameWorld.h"
#include "Render/Render.h"

BE_NAMESPACE_BEGIN

const SignalDef ComRectTransform::SIG_RectTransformUpdated("ComRectTransform::RectTransformUpdated", "a");

OBJECT_DECLARATION("Rect Transform", ComRectTransform, ComTransform)
BEGIN_EVENTS(ComRectTransform)
END_EVENTS

void ComRectTransform::RegisterProperties() {
    REGISTER_MIXED_ACCESSOR_PROPERTY("anchorMin", "Anchor Min", Vec2, GetAnchorMin, SetAnchorMin, Vec2(0.5, 0.5),
        "", PropertyInfo::Flag::Editor);
    REGISTER_MIXED_ACCESSOR_PROPERTY("anchorMax", "Anchor Max", Vec2, GetAnchorMax, SetAnchorMax, Vec2(0.5, 0.5),
        "", PropertyInfo::Flag::Editor);
    REGISTER_MIXED_ACCESSOR_PROPERTY("anchoredPosition", "Anchored Position", Vec2, GetAnchoredPosition, SetAnchoredPosition, Vec2(0, 0),
        "", PropertyInfo::Flag::Editor);
    REGISTER_MIXED_ACCESSOR_PROPERTY("sizeDelta", "Size Delta", Vec2, GetSizeDelta, SetSizeDelta, Vec2(100, 100),
        "", PropertyInfo::Flag::Editor);
    REGISTER_MIXED_ACCESSOR_PROPERTY("pivot", "Pivot", Vec2, GetPivot, SetPivot, Vec2(0.5, 0.5),
        "", PropertyInfo::Flag::Editor);
}

ComRectTransform::ComRectTransform() {
    cachedRect = RectF::empty;
    cachedRectInvalidated = true;
}

ComRectTransform::~ComRectTransform() {
}

void ComRectTransform::Init() {
    Component::Init();

    UpdateWorldMatrix();

    // Mark as initialized
    SetInitialized(true);
}

Vec2 ComRectTransform::GetAnchorMin() const {
    return anchorMin;
}

void ComRectTransform::SetAnchorMin(const Vec2 &anchorMin) {
    this->anchorMin = anchorMin;

    if (IsInitialized()) {
        InvalidateCachedRect();
    }
}

Vec2 ComRectTransform::GetAnchorMax() const {
    return anchorMax;
}

void ComRectTransform::SetAnchorMax(const Vec2 &anchorMax) {
    this->anchorMax = anchorMax;

    if (IsInitialized()) {
        InvalidateCachedRect();
    }
}

Vec2 ComRectTransform::GetAnchoredPosition() const {
    return anchoredPosition;
}

void ComRectTransform::SetAnchoredPosition(const Vec2 &anchoredPosition) {
    this->anchoredPosition = anchoredPosition;

    if (IsInitialized()) {
        InvalidateCachedRect();
    }
}

Vec2 ComRectTransform::GetSizeDelta() const {
    return sizeDelta;
}

void ComRectTransform::SetSizeDelta(const Vec2 &sizeDelta) {
    this->sizeDelta = sizeDelta;

    if (IsInitialized()) {
        InvalidateCachedRect();
    }
}

Vec2 ComRectTransform::GetPivot() const {
    return pivot;
}

void ComRectTransform::SetPivot(const Vec2 &pivot) {
    this->pivot = pivot;

    if (IsInitialized()) {
        InvalidateCachedRect();
    }
}

Vec3 ComRectTransform::GetLocalOrigin() const {
    if (cachedRectInvalidated) {
        UpdateLocalOriginAndRect();
    }
    return ComTransform::GetLocalOrigin();
}

Vec3 ComRectTransform::GetOrigin() const {
    if (cachedRectInvalidated) {
        UpdateLocalOriginAndRect();
    }
    return ComTransform::GetOrigin();
}

RectF ComRectTransform::GetLocalRect() const {
    if (cachedRectInvalidated) {
        UpdateLocalOriginAndRect();
    }
    return cachedRect;
}

void ComRectTransform::GetLocalCorners(Vec3 (&localCorners)[4]) const {
    RectF rect = GetLocalRect();
    
    float x1 = rect.x;
    float y1 = rect.y;
    float x2 = rect.X2();
    float y2 = rect.Y2();

    localCorners[0] = Vec3(x1, y1, 0);
    localCorners[1] = Vec3(x2, y1, 0);
    localCorners[2] = Vec3(x2, y2, 0);
    localCorners[3] = Vec3(x1, y2, 0);
}

void ComRectTransform::GetWorldCorners(Vec3 (&worldCorners)[4]) const {
    Vec3 localCorners[4];
    GetLocalCorners(localCorners);

    Mat3x4 worldMatrix = GetMatrix();

    for (int i = 0; i < 4; i++) {
        worldCorners[i] = worldMatrix.Transform(localCorners[i]);
    }
}

void ComRectTransform::GetWorldAnchorCorners(Vec3 (&worldAnchorCorners)[4]) const {
    for (int i = 0; i < COUNT_OF(worldAnchorCorners); i++) {
        worldAnchorCorners[i] = Vec3::zero;
    }

    ComTransform *parentTransform = GetParent();
    if (parentTransform) {
        ComRectTransform *parentRectTransform = parentTransform->Cast<ComRectTransform>();
        if (parentRectTransform) {
            Vec3 parentWorldCorners[4];
            parentRectTransform->GetWorldCorners(parentWorldCorners);

            Vec2 worldAnchorMin, worldAnchorMax;
            worldAnchorMin.x = parentWorldCorners[0].x + (parentWorldCorners[2].x - parentWorldCorners[0].x) * anchorMin.x;
            worldAnchorMin.y = parentWorldCorners[0].y + (parentWorldCorners[2].y - parentWorldCorners[0].y) * anchorMin.y;
            worldAnchorMax.x = parentWorldCorners[0].x + (parentWorldCorners[2].x - parentWorldCorners[0].x) * anchorMax.x;
            worldAnchorMax.y = parentWorldCorners[0].y + (parentWorldCorners[2].y - parentWorldCorners[0].y) * anchorMax.y;

            worldAnchorCorners[0] = Vec3(worldAnchorMin.x, worldAnchorMin.y, 0);
            worldAnchorCorners[1] = Vec3(worldAnchorMax.x, worldAnchorMin.y, 0);
            worldAnchorCorners[2] = Vec3(worldAnchorMax.x, worldAnchorMax.y, 0);
            worldAnchorCorners[3] = Vec3(worldAnchorMin.x, worldAnchorMax.y, 0);
        }
    }
}

Vec3 ComRectTransform::GetWorldPivot() const {
    return LocalPivotToWorld(pivot);
}

Vec3 ComRectTransform::LocalPivotToWorld(const Vec2 &localPivot) const {
    Vec3 worldCorners[4];
    GetWorldCorners(worldCorners);

    return GetWorldPivotInWorldRect(worldCorners, localPivot);
}

Vec2 ComRectTransform::WorldPivotToLocal(const Vec3 &worldPivot) const {
    Vec3 worldCorners[4];
    GetWorldCorners(worldCorners);

    return GetLocalPivotInWorldRect(worldCorners, worldPivot);
}

Vec3 ComRectTransform::GetWorldPivotInWorldRect(const Vec3(&worldCorners)[4], const Vec2 &localPivot) {
    Vec3 b = Lerp(worldCorners[0], worldCorners[1], localPivot.x);
    Vec3 t = Lerp(worldCorners[3], worldCorners[2], localPivot.x);

    return Lerp(b, t, localPivot.y);
}

Vec2 ComRectTransform::GetLocalPivotInWorldRect(const Vec3 (&worldCorners)[4], const Vec3 &worldPivot) {
    // TODO: Check world pivot should be on the plane of rectangle.
    Vec3 pivotDir = worldPivot - worldCorners[0];

    Vec3 xAxis = worldCorners[1] - worldCorners[0];
    Vec3 yAxis = worldCorners[3] - worldCorners[0];

    float xLength = xAxis.Normalize();
    float yLength = yAxis.Normalize();

    Vec2 localPivot;
    localPivot.x = pivotDir.Dot(xAxis) / xLength;
    localPivot.y = pivotDir.Dot(yAxis) / yLength;

    return localPivot;
}

bool ComRectTransform::RayToWorldPointInRectangle(const Ray &ray, Vec3 &worldPoint) const {
    Plane plane(GetRotation() * Vec3::unitZ, GetOrigin());

    float hitDist;
    if (!plane.IntersectRay(ray, false, &hitDist)) {
        return false;
    }

    worldPoint = ray.GetPoint(hitDist);
    return true;
}

bool ComRectTransform::RayToLocalPointInRectangle(const Ray &ray, Vec2 &localPoint) const {
    localPoint = Vec2::zero;

    Vec3 worldPoint;
    if (!RayToWorldPointInRectangle(ray, worldPoint)) {
        return false;
    }

    // CHECK!!!
    localPoint = GetMatrixNoScale().ToMat3().TransposedMulVec(worldPoint - GetOrigin()).ToVec2();
    return true;
}

bool ComRectTransform::IsLocalPointInRect(const Vec2 &localPoint) const {
    RectF localRect = GetLocalRect();

    return localRect.IsContainPoint(localPoint.x, localPoint.y);
}

void ComRectTransform::UpdateLocalOriginAndRect() const {
    Vec3 oldLocalOrigin = ComTransform::GetLocalOrigin();
    Vec3 newLocalOrigin = ComputeLocalOrigin3D();

    if (oldLocalOrigin != newLocalOrigin) {
#if WITH_EDITOR
        const_cast<ComRectTransform *>(this)->SetProperty("origin", newLocalOrigin);
#else
        const_cast<ComRectTransform *>(this)->SetLocalOrigin(newLocalOrigin);
#endif
    }

    // Since the local origin has been recalculated, the pivot rect means local rect.
    cachedRect = ComputePivotRect();

    cachedRectInvalidated = false;
}

void ComRectTransform::InvalidateCachedRect() {
    if (cachedRectInvalidated) {
        return;
    }
    cachedRectInvalidated = true;

    // World matrix will be updated so it's safe to emit this signal.
    EmitSignal(&SIG_RectTransformUpdated, this);

    for (Entity *childEntity = GetEntity()->GetNode().GetFirstChild(); childEntity; childEntity = childEntity->GetNode().GetNextSibling()) {
        ComRectTransform *rectTransform = childEntity->GetComponent(0)->Cast<ComRectTransform>();
        if (rectTransform) {
            rectTransform->InvalidateCachedRect();
        }
    }
}

RectF ComRectTransform::ComputeLocalRect() const {
    RectF parentRect = RectF(0, 0, 0, 0);

    ComTransform *parentTransform = GetParent();
    if (parentTransform) {
        ComRectTransform *parentRectTransform = parentTransform->Cast<ComRectTransform>();
        if (parentRectTransform) {
            parentRect = parentRectTransform->GetLocalRect();
        }
    }

    // Calculate anchored minimum position in parent space.
    PointF localMins = PointF(
        parentRect.x + (parentRect.w * anchorMin.x), 
        parentRect.y + (parentRect.h * anchorMin.y));

    // Calculate anchored maximum position in parent space.
    PointF localMaxs = PointF(
        parentRect.x + (parentRect.w * anchorMax.x), 
        parentRect.y + (parentRect.h * anchorMax.y));

    // Calculate rectangle in parent space.
    RectF localRect;
    localRect.x = localMins.x + anchoredPosition.x - (sizeDelta.x * pivot.x);
    localRect.y = localMins.y + anchoredPosition.y - (sizeDelta.y * pivot.y);
    localRect.w = localMaxs.x - localMins.x + sizeDelta.x;
    localRect.h = localMaxs.y - localMins.y + sizeDelta.y;

    return localRect;
}

RectF ComRectTransform::ComputePivotRect() const {
    RectF rect = ComputeLocalRect();
    
    // Translate rect in local pivot.
    rect.x = -rect.w * pivot.x;
    rect.y = -rect.h * pivot.y;

    return rect;
}

Vec2 ComRectTransform::ComputeLocalOrigin2D() const {
    RectF rect = ComputeLocalRect();

    return Vec2(
        rect.x + rect.w * pivot.x,
        rect.y + rect.h * pivot.y);
}

Vec3 ComRectTransform::ComputeLocalOrigin3D() const {
    RectF rect = ComputeLocalRect();
    Vec3 localOrigin = ComTransform::GetLocalOrigin();

    return Vec3(
        rect.x + rect.w * pivot.x,
        rect.y + rect.h * pivot.y,
        localOrigin.z);
}

#if WITH_EDITOR
void ComRectTransform::DrawGizmos(const RenderCamera *camera, bool selected, bool selectedByParent) {
    bool hasCanvas = GetEntity()->GetComponent<ComCanvas>() ? true : false;

    if (selected || hasCanvas) {
        RenderWorld *renderWorld = GetGameWorld()->GetRenderWorld();

        // Draw rectangle
        Vec3 worldCorners[4];
        GetWorldCorners(worldCorners);

        renderWorld->SetDebugColor(Color4(1.0f, 1.0f, 1.0f, 0.5f), Color4::zero);

        renderWorld->DebugLine(worldCorners[0], worldCorners[1]);
        renderWorld->DebugLine(worldCorners[1], worldCorners[2]);
        renderWorld->DebugLine(worldCorners[2], worldCorners[3]);
        renderWorld->DebugLine(worldCorners[3], worldCorners[0]);

        if (!hasCanvas) {
            ComTransform *parentTransform = GetParent();
            if (parentTransform) {
                ComRectTransform *parentRectTransform = parentTransform->Cast<ComRectTransform>();
                if (parentRectTransform) {
                    // Draw parent rectangle
                    Vec3 parentWorldCorners[4];
                    parentRectTransform->GetWorldCorners(parentWorldCorners);

                    renderWorld->SetDebugColor(Color4(1.0f, 1.0f, 1.0f, 1.0f), Color4::zero);

                    renderWorld->DebugLine(parentWorldCorners[0], parentWorldCorners[1]);
                    renderWorld->DebugLine(parentWorldCorners[1], parentWorldCorners[2]);
                    renderWorld->DebugLine(parentWorldCorners[2], parentWorldCorners[3]);
                    renderWorld->DebugLine(parentWorldCorners[3], parentWorldCorners[0]);

                    // Draw anchors
                    Vec3 worldAnchorCorners[4];
                    GetWorldAnchorCorners(worldAnchorCorners);

                    float viewScale = camera->CalcViewScale(worldAnchorCorners[0]);
                    renderWorld->DebugTriangle(worldAnchorCorners[0], worldAnchorCorners[0] + Vec3(-10, -5, 0) * viewScale, worldAnchorCorners[0] + Vec3(-5, -10, 0) * viewScale);

                    viewScale = camera->CalcViewScale(worldAnchorCorners[1]);
                    renderWorld->DebugTriangle(worldAnchorCorners[1], worldAnchorCorners[1] + Vec3(+10, -5, 0) * viewScale, worldAnchorCorners[1] + Vec3(+5, -10, 0) * viewScale);

                    viewScale = camera->CalcViewScale(worldAnchorCorners[2]);
                    renderWorld->DebugTriangle(worldAnchorCorners[2], worldAnchorCorners[2] + Vec3(+10, +5, 0) * viewScale, worldAnchorCorners[2] + Vec3(+5, +10, 0) * viewScale);

                    viewScale = camera->CalcViewScale(worldAnchorCorners[3]);
                    renderWorld->DebugTriangle(worldAnchorCorners[3], worldAnchorCorners[3] + Vec3(-10, +5, 0) * viewScale, worldAnchorCorners[3] + Vec3(-5, +10, 0) * viewScale);
                }
            }
        }
    }
}
#endif

const AABB ComRectTransform::GetAABB() const {
    Vec3 localCorners[4];
    GetLocalCorners(localCorners);

    AABB aabb;
    aabb.Clear();
    aabb.AddPoint(localCorners[0]);
    aabb.AddPoint(localCorners[1]);
    aabb.AddPoint(localCorners[2]);
    aabb.AddPoint(localCorners[3]);

    return aabb;
}

BE_NAMESPACE_END
