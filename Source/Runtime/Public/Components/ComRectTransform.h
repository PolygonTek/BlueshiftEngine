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

    Vec2                    GetAnchorMin() const;
    void                    SetAnchorMin(const Vec2 &anchorMin);

    Vec2                    GetAnchorMax() const;
    void                    SetAnchorMax(const Vec2 &anchorMax);

    Vec2                    GetAnchoredPosition() const;
    void                    SetAnchoredPosition(const Vec2 &anchoredPosition);

    Vec2                    GetSizeDelta() const;
    void                    SetSizeDelta(const Vec2 &sizeDelta);

    Vec2                    GetPivot() const;
    void                    SetPivot(const Vec2 &pivot);

    RectF                   GetRectInLocalSpace();

protected:
    void                    UpdateOriginAndRect();
    void                    InvalidateCachedRect();
    RectF                   ComputeRectInParentSpace() const;
    RectF                   ComputeRectInLocalSpace() const;
    Vec2                    ComputeLocalOrigin2D() const;
    Vec3                    ComputeLocalOrigin3D() const;

    Vec2                    anchorMin;
    Vec2                    anchorMax;
    Vec2                    anchoredPosition;
    Vec2                    sizeDelta;
    Vec2                    pivot;

    mutable bool            cachedRectInvalidated;
    mutable RectF           cachedRect;
};

BE_NAMESPACE_END
