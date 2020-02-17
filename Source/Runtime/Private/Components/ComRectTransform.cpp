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
    REGISTER_MIXED_ACCESSOR_PROPERTY("anchorMins", "Anchor Mins", Vec2, GetAnchorMins, SetAnchorMins, Vec2(0.5, 0.5),
        "", PropertyInfo::Flag::Editor);
    REGISTER_MIXED_ACCESSOR_PROPERTY("anchorMaxs", "Anchor Maxs", Vec2, GetAnchorMaxs, SetAnchorMaxs, Vec2(0.5, 0.5),
        "", PropertyInfo::Flag::Editor);
    REGISTER_MIXED_ACCESSOR_PROPERTY("anchoredPosition", "Anchored Position", Vec2, GetAnchoredPosition, SetAnchoredPosition, Vec2(0, 0),
        "", PropertyInfo::Flag::Editor);
    REGISTER_MIXED_ACCESSOR_PROPERTY("sizeDelta", "Size Delta", Vec2, GetSizeDelta, SetSizeDelta, Vec2(100, 100),
        "", PropertyInfo::Flag::Editor);
    REGISTER_MIXED_ACCESSOR_PROPERTY("pivot", "Pivot", Vec2, GetPivot, SetPivot, Vec2(0.5, 0.5),
        "", PropertyInfo::Flag::Editor);
    REGISTER_PROPERTY("rayCastTarget", "Ray Cast Target", bool, rayCastTarget, true,
        "", PropertyInfo::Flag::Editor);
}

ComRectTransform::ComRectTransform() {
    cachedRect = RectF::zero;
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

Vec2 ComRectTransform::GetAnchorMins() const {
    return anchorMins;
}

void ComRectTransform::SetAnchorMins(const Vec2 &anchorMins) {
    this->anchorMins = anchorMins;

    if (IsInitialized()) {
        InvalidateCachedRect();
    }
}

Vec2 ComRectTransform::GetAnchorMaxs() const {
    return anchorMaxs;
}

void ComRectTransform::SetAnchorMaxs(const Vec2 &anchorMaxs) {
    this->anchorMaxs = anchorMaxs;

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

    localCorners[0] = Coords2D::To3D(x1, y1);
    localCorners[1] = Coords2D::To3D(x2, y1);
    localCorners[2] = Coords2D::To3D(x2, y2);
    localCorners[3] = Coords2D::To3D(x1, y2);
}

void ComRectTransform::GetWorldCorners(Vec3 (&worldCorners)[4]) const {
    Vec3 localCorners[4];
    GetLocalCorners(localCorners);

    ALIGN_AS16 Mat3x4 worldMatrix = GetMatrix();

    for (int i = 0; i < COUNT_OF(localCorners); i++) {
        worldCorners[i] = worldMatrix.Transform(localCorners[i]);
    }
}

void ComRectTransform::GetWorldAnchorCorners(Vec3 (&worldAnchorCorners)[4]) const {
    const ComTransform *parentTransform = GetParent();
    while (parentTransform) {
        const ComRectTransform *parentRectTransform = parentTransform->Cast<ComRectTransform>();

        if (parentRectTransform) {
            worldAnchorCorners[0] = parentRectTransform->NormalizedPosToWorld(Vec2(anchorMins.x, anchorMins.y));
            worldAnchorCorners[1] = parentRectTransform->NormalizedPosToWorld(Vec2(anchorMaxs.x, anchorMins.y));
            worldAnchorCorners[2] = parentRectTransform->NormalizedPosToWorld(Vec2(anchorMaxs.x, anchorMaxs.y));
            worldAnchorCorners[3] = parentRectTransform->NormalizedPosToWorld(Vec2(anchorMins.x, anchorMaxs.y));
            return;
        }

        parentTransform = parentTransform->GetParent();
    }

    worldAnchorCorners[0] = Vec3::zero;
    worldAnchorCorners[1] = Vec3::zero;
    worldAnchorCorners[2] = Vec3::zero;
    worldAnchorCorners[3] = Vec3::zero;
}

Vec3 ComRectTransform::GetWorldAnchorMins() const {
    const ComTransform *parentTransform = GetParent();
    while (parentTransform) {
        const ComRectTransform *parentRectTransform = parentTransform->Cast<ComRectTransform>();

        if (parentRectTransform) {
            return parentRectTransform->NormalizedPosToWorld(anchorMins);
        }

        parentTransform = parentTransform->GetParent();
    }
    return Vec3::zero;
}

Vec3 ComRectTransform::GetWorldAnchorMaxs() const {
    const ComTransform *parentTransform = GetParent();
    while (parentTransform) {
        const ComRectTransform *parentRectTransform = parentTransform->Cast<ComRectTransform>();

        if (parentRectTransform) {
            return parentRectTransform->NormalizedPosToWorld(anchorMaxs);
        }

        parentTransform = parentTransform->GetParent();
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
    Vec3 b = Math::Lerp(worldCorners[0], worldCorners[1], normalizedPos.x);
    Vec3 t = Math::Lerp(worldCorners[3], worldCorners[2], normalizedPos.x);

    return Math::Lerp(b, t, normalizedPos.y);
}

Vec2 ComRectTransform::ComputeNormalizedPositionFromWorld(const Vec3 (&worldCorners)[4], const Vec3 &worldPos) {
    Vec3 worldPosOnPlane = worldPos;//Plane::FromPoints(worldCorners[0], worldCorners[1], worldCorners[2]).Project(worldPos);
    Vec3 dir = worldPosOnPlane - worldCorners[0];

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
    Plane plane(GetRotation() * Coords2D::ZAxis(), GetOrigin());

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

    localPoint = Coords2D::From3D(GetMatrixNoScale().ToMat3().TransposedMulVec(worldPoint - GetOrigin()));
    return true;
}

bool ComRectTransform::IsLocalPointInRectangle(const Vec2 &localPoint) const {
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
        ComTransform *transform = childEntity->GetTransform();

        if (transform) {
            transform->InvalidateCachedRect();
        }
    }
}

RectF ComRectTransform::ComputeLocalRect() const {
    RectF parentRect = RectF(0, 0, 0, 0);

    ComTransform *parentTransform = GetParent();
    while (parentTransform) {
        ComRectTransform *parentRectTransform = parentTransform->Cast<ComRectTransform>();

        if (parentRectTransform) {
            parentRect = parentRectTransform->GetLocalRect();
            break;
        }

        parentTransform = parentTransform->GetParent();
    }

    // Calculate anchored minimum position in parent space.
    PointF localMins = PointF(
        parentRect.x + (parentRect.w * anchorMins.x), 
        parentRect.y + (parentRect.h * anchorMins.y));

    // Calculate anchored maximum position in parent space.
    PointF localMaxs = PointF(
        parentRect.x + (parentRect.w * anchorMaxs.x), 
        parentRect.y + (parentRect.h * anchorMaxs.y));

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

    float x = rect.x + rect.w * pivot.x;
    float y = rect.y + rect.h * pivot.y;

    return Vec2(x, y);
}

Vec3 ComRectTransform::ComputeLocalOrigin3D() const {
    RectF rect = ComputeLocalRect();

    float x = rect.x + rect.w * pivot.x;
    float y = rect.y + rect.h * pivot.y;
    float z = Coords2D::ZFrom3D(ComTransform::GetLocalOrigin());

    return Coords2D::To3D(x, y, z);
}

#if WITH_EDITOR
void ComRectTransform::DrawGizmos(const RenderCamera *camera, bool selected, bool selectedByParent) {
    bool hasCanvas = GetEntity()->GetComponent<ComCanvas>() ? true : false;

    if (selected) {
        RenderWorld *renderWorld = GetGameWorld()->GetRenderWorld();

        // Draw selected rectangle.
        Vec3 worldCorners[4];
        GetWorldCorners(worldCorners);

        renderWorld->SetDebugColor(Color4(1.0f, 1.0f, 1.0f, 1.0f), Color4::zero);

        renderWorld->DebugLine(worldCorners[0], worldCorners[1], 1.0f, true);
        renderWorld->DebugLine(worldCorners[1], worldCorners[2], 1.0f, true);
        renderWorld->DebugLine(worldCorners[2], worldCorners[3], 1.0f, true);
        renderWorld->DebugLine(worldCorners[3], worldCorners[0], 1.0f, true);

        ComTransform *parentTransform = GetParent();
        while (parentTransform) {
            ComRectTransform *parentRectTransform = parentTransform->Cast<ComRectTransform>();
            if (parentRectTransform) {
                // Draw parent rectangle.
                Vec3 parentWorldCorners[4];
                parentRectTransform->GetWorldCorners(parentWorldCorners);

                renderWorld->SetDebugColor(Color4(0.0f, 1.0f, 0.75f, 1.0f), Color4::zero);

                renderWorld->DebugLine(parentWorldCorners[0], parentWorldCorners[1], 1.0f, true);
                renderWorld->DebugLine(parentWorldCorners[1], parentWorldCorners[2], 1.0f, true);
                renderWorld->DebugLine(parentWorldCorners[2], parentWorldCorners[3], 1.0f, true);
                renderWorld->DebugLine(parentWorldCorners[3], parentWorldCorners[0], 1.0f, true);
                break;
            }

            parentTransform = parentTransform->GetParent();
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
