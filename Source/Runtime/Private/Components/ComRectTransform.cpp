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
#include "Game/Entity.h"

BE_NAMESPACE_BEGIN

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

RectF ComRectTransform::GetRectInLocalSpace() {
    if (cachedRectInvalidated) {
        UpdateOriginAndRect();
    }
    return cachedRect;
}

void ComRectTransform::UpdateOriginAndRect() {
    Vec3 oldLocalOrigin = GetLocalOrigin();
    Vec3 newLocalOrigin = ComputeLocalOrigin3D();

    if (oldLocalOrigin != newLocalOrigin) {
        SetLocalOrigin(newLocalOrigin);
    }

    cachedRect = ComputeRectInLocalSpace();

    cachedRectInvalidated = false;
}

void ComRectTransform::InvalidateCachedRect() {
    if (cachedRectInvalidated) {
        return;
    }
    cachedRectInvalidated = true;

    for (Entity *childEntity = GetEntity()->GetNode().GetFirstChild(); childEntity; childEntity = childEntity->GetNode().GetNextSibling()) {
        ComRectTransform *rectTransform = childEntity->GetComponent(0)->Cast<ComRectTransform>();
        if (rectTransform) {
            rectTransform->InvalidateCachedRect();
        }
    }
}

RectF ComRectTransform::ComputeRectInParentSpace() const {
    RectF parentRect = RectF(0, 0, 0, 0);

    // Canvas don't have parent transform.
    ComTransform *parentTransform = GetParent();
    if (parentTransform) {
        ComRectTransform *parentRectTransform = parentTransform->Cast<ComRectTransform>();
        if (parentRectTransform) {
            parentRect = parentRectTransform->GetRectInLocalSpace();
        }
    }

    PointF anchoredMin = PointF(
        parentRect.x + (parentRect.w * anchorMin.x), 
        parentRect.y + (parentRect.h * anchorMin.y));

    PointF anchoredMax = PointF(
        parentRect.x + (parentRect.w * anchorMax.x), 
        parentRect.y + (parentRect.h * anchorMax.y));

    RectF rect;
    rect.x = anchoredMin.x + anchoredPosition.x - (sizeDelta.x * pivot.x);
    rect.y = anchoredMin.y + anchoredPosition.y - (sizeDelta.y * pivot.y);
    rect.w = anchoredMax.x - anchoredMin.x + sizeDelta.x;
    rect.h = anchoredMax.y - anchoredMin.y + sizeDelta.y;

    return rect;
}

RectF ComRectTransform::ComputeRectInLocalSpace() const {
    RectF rect = ComputeRectInParentSpace();
    
    // Translate rect in local pivot.
    rect.x = -rect.w * pivot.x;
    rect.y = -rect.h * pivot.y;

    return rect;
}

// Returns local origin in parent space in 2D.
Vec2 ComRectTransform::ComputeLocalOrigin2D() const {
    RectF rect = ComputeRectInParentSpace();

    return Vec2(
        rect.x + rect.w * pivot.x,
        rect.y + rect.h * pivot.y);
}

// Returns local origin in parent space in 3D.
Vec3 ComRectTransform::ComputeLocalOrigin3D() const {
    RectF rect = ComputeRectInParentSpace();
    Vec3 localOrigin = GetLocalOrigin();

    return Vec3(
        rect.x + rect.w * pivot.x,
        rect.y + rect.h * pivot.y,
        localOrigin.z);
}

BE_NAMESPACE_END
