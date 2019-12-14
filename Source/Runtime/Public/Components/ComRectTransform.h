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

#pragma once

#include "ComTransform.h"

BE_NAMESPACE_BEGIN

class ComRectTransform : public ComTransform {
public:
    OBJECT_PROTOTYPE(ComRectTransform);
    
    ComRectTransform();
    virtual ~ComRectTransform();

                            /// Initializes this component. Called after deserialization.
    virtual void            Init() override;

                            /// RectTransform component cannot be disabled.
    virtual bool            CanDisable() const override { return false; }

#if WITH_EDITOR
                            /// Visualize the component in editor
    virtual void            DrawGizmos(const RenderCamera *camera, bool selected, bool selectedByParent) override;
#endif

    virtual const AABB      GetAABB() const override;

    Vec2                    GetAnchorMins() const;
    void                    SetAnchorMins(const Vec2 &anchorMins);

    Vec2                    GetAnchorMaxs() const;
    void                    SetAnchorMaxs(const Vec2 &anchorMaxs);

    Vec2                    GetAnchoredPosition() const;
    void                    SetAnchoredPosition(const Vec2 &anchoredPosition);

    Vec2                    GetSizeDelta() const;
    void                    SetSizeDelta(const Vec2 &sizeDelta);

    Vec2                    GetPivot() const;
    void                    SetPivot(const Vec2 &pivot);

    bool                    IsRayCastTarget() const { return rayCastTarget; }

                            /// Returns position in world space (overrided).
    Vec3                    GetOrigin() const;

                            /// Returns position in local space (overrided).
    Vec3                    GetLocalOrigin() const;

                            /// Computes rectangle in local space.
    RectF                   GetLocalRect() const;

                            /// Computes rectangular vertices in local space.
    void                    GetLocalCorners(Vec3 (&corners)[4]) const;

                            /// Computes rectangular vertices in world space.
    void                    GetWorldCorners(Vec3 (&corners)[4]) const;

                            /// Computes anchor coordinates of rectangle in world space.
    void                    GetWorldAnchorCorners(Vec3 (&worldAnchorCorners)[4]) const;

                            /// Computes minimum anchor coordinates of rectangle in world space.
    Vec3                    GetWorldAnchorMins() const;

                            /// Computes maximum anchor coordinates of rectangle in world space.
    Vec3                    GetWorldAnchorMaxs() const;

                            /// Returns pivot position in world space.
    Vec3                    GetWorldPivot() const;

                            /// Returns anchored position in world space.
    Vec3                    GetWorldAnchoredPosition() const;

                            /// Converts normalized position of rectangle to world space position.
    Vec3                    NormalizedPosToWorld(const Vec2 &normalizedPosition) const;

                            /// Converts world space position to normalized position of rectangle.
    Vec2                    WorldPosToNormalized(const Vec3 &worldPosition) const;

                            /// Compute world space position from normalized position of rectangle.
    static Vec3             ComputeWorldPositionFromNormalized(const Vec3(&worldCorners)[4], const Vec2 &localPosition);

                            /// Compute normalized position of rectangle from world space position.
    static Vec2             ComputeNormalizedPositionFromWorld(const Vec3(&worldCorners)[4], const Vec3 &worldPosition);

                            /// Computes ray intersection point in world space of the rectangle.
    bool                    RayToWorldPointInRectangle(const Ray &ray, Vec3 &worldPoint) const;

                            /// Computes ray intersection point in local space of the rectangle.
    bool                    RayToLocalPointInRectangle(const Ray &ray, Vec2 &localPoint) const;

                            /// Is the point in local space inside the rectangle ?
    bool                    IsLocalPointInRect(const Vec2 &localPoint) const;

    static const SignalDef  SIG_RectTransformUpdated;

protected:
                            /// Updates local origin and (cached) rect.
    void                    UpdateLocalOriginAndRect() const;

                            /// Marks this RectTransform and children to need cached rect recalculation.
    void                    InvalidateCachedRect();

                            /// Computes rectangle in local space.
    RectF                   ComputeLocalRect() const;

                            /// Computes rectangle in pivot space.
    RectF                   ComputePivotRect() const;

                            /// Computes local origin in parent space in 2D.
    Vec2                    ComputeLocalOrigin2D() const;

                            /// Computes local origin in parent space in 3D.
    Vec3                    ComputeLocalOrigin3D() const;

    Vec2                    anchorMins;
    Vec2                    anchorMaxs;
    Vec2                    anchoredPosition;
    Vec2                    sizeDelta;
    Vec2                    pivot;
    bool                    rayCastTarget;

    mutable bool            cachedRectInvalidated;
    mutable RectF           cachedRect;                 ///< Cached rectangle in local space.
};

BE_NAMESPACE_END
