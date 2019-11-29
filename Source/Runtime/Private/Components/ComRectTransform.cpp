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
    const Entity *parentEnt = GetEntity()->GetParent();
    if (parentEnt) {
        const ComRectTransform *parentRectTransform = parentEnt->GetRectTransform();

        if (parentRectTransform) {
            worldAnchorCorners[0] = parentRectTransform->NormalizedPosToWorld(Vec2(anchorMin.x, anchorMin.y));
            worldAnchorCorners[1] = parentRectTransform->NormalizedPosToWorld(Vec2(anchorMax.x, anchorMin.y));
            worldAnchorCorners[2] = parentRectTransform->NormalizedPosToWorld(Vec2(anchorMax.x, anchorMax.y));
            worldAnchorCorners[3] = parentRectTransform->NormalizedPosToWorld(Vec2(anchorMin.x, anchorMax.y));
            return;
        } else {
            worldAnchorCorners[0] = parentEnt->GetTransform()->GetOrigin();
            worldAnchorCorners[1] = parentEnt->GetTransform()->GetOrigin();
            worldAnchorCorners[2] = parentEnt->GetTransform()->GetOrigin();
            worldAnchorCorners[3] = parentEnt->GetTransform()->GetOrigin();
            return;
        }
    }

    worldAnchorCorners[0] = Vec3::zero;
    worldAnchorCorners[1] = Vec3::zero;
    worldAnchorCorners[2] = Vec3::zero;
    worldAnchorCorners[3] = Vec3::zero;
}

Vec3 ComRectTransform::GetWorldAnchorMin() const {
    const Entity *parentEnt = GetEntity()->GetParent();
    if (parentEnt) {
        const ComRectTransform *parentRectTransform = parentEnt->GetRectTransform();

        if (parentRectTransform) {
            return parentRectTransform->NormalizedPosToWorld(anchorMin);
        }
    }
    return Vec3::zero;
}

Vec3 ComRectTransform::GetWorldAnchorMax() const {
    const Entity *parentEnt = GetEntity()->GetParent();
    if (parentEnt) {
        const ComRectTransform *parentRectTransform = parentEnt->GetRectTransform();

        if (parentRectTransform) {
            return parentRectTransform->NormalizedPosToWorld(anchorMax);
        }
    }
    return Vec3::zero;
}

Vec3 ComRectTransform::GetWorldPivot() const {
    return NormalizedPosToWorld(pivot);
}

Vec3 ComRectTransform::NormalizedPosToWorld(const Vec2 &normalizedPosition) const {
    Vec3 worldCorners[4];
    GetWorldCorners(worldCorners);

    return ComputeWorldPositionFromNormalized(worldCorners, normalizedPosition);
}

Vec2 ComRectTransform::WorldPosToNormalized(const Vec3 &worldPosition) const {
    Vec3 worldCorners[4];
    GetWorldCorners(worldCorners);

    return ComputeNormalizedPositionFromWorld(worldCorners, worldPosition);
}

Vec3 ComRectTransform::ComputeWorldPositionFromNormalized(const Vec3(&worldCorners)[4], const Vec2 &normalizedPos) {
    Vec3 b = Lerp(worldCorners[0], worldCorners[1], normalizedPos.x);
    Vec3 t = Lerp(worldCorners[3], worldCorners[2], normalizedPos.x);

    return Lerp(b, t, normalizedPos.y);
}

Vec2 ComRectTransform::ComputeNormalizedPositionFromWorld(const Vec3 (&worldCorners)[4], const Vec3 &worldPos) {
    // TODO: Check worldPos should be on the plane of rectangle.
    Vec3 dir = worldPos - worldCorners[0];

    Vec3 xAxis = worldCorners[1] - worldCorners[0];
    Vec3 yAxis = worldCorners[3] - worldCorners[0];

    float xLength = xAxis.Normalize();
    float yLength = yAxis.Normalize();

    Vec2 normalizedPos;
    normalizedPos.x = dir.Dot(xAxis) / xLength;
    normalizedPos.y = dir.Dot(yAxis) / yLength;

    return normalizedPos;
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
