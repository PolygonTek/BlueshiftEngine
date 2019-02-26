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

#include "Component.h"

BE_NAMESPACE_BEGIN

class ComTransform;

class ComReflectionProbe : public Component {
public:
    OBJECT_PROTOTYPE(ComReflectionProbe);

    ComReflectionProbe();
    virtual ~ComReflectionProbe();

    virtual void            Purge(bool chainPurge = true) override;

                            /// Initializes this component. Called after deserialization.
    virtual void            Init() override;

    virtual bool            HasRenderEntity(int renderEntityHandle) const override;

    virtual bool            RayIntersection(const Vec3 &start, const Vec3 &dir, bool backFaceCull, float &lastScale) const override;

                            /// Visualize the component in editor
    virtual void            DrawGizmos(const RenderCamera::State &viewState, bool selected) override;

    virtual const AABB      GetAABB() override;

    ReflectionProbe::Type   GetType() const;
    void                    SetType(ReflectionProbe::Type type);

    int                     GetImportance() const;
    void                    SetImportance(int importance);

    ReflectionProbe::Resolution GetResolution() const;
    void                    SetResolution(ReflectionProbe::Resolution resolution);

    bool                    IsHDR() const;
    void                    SetHDR(bool useHDR);

    ReflectionProbe::ClearMethod GetClearMethod() const;
    void                    SetClearMethod(ReflectionProbe::ClearMethod clearMethod);

    Color3                  GetClearColor() const;
    void                    SetClearColor(const Color3 &clearColor);

    float                   GetClearAlpha() const;
    void                    SetClearAlpha(float clearAlpha);

    float                   GetClippingNear() const;
    void                    SetClippingNear(float clippingNear);

    float                   GetClippingFar() const;
    void                    SetClippingFar(float clippingFar);

    bool                    IsBoxProjection() const;
    void                    SetBoxProjection(bool useBoxProjection);

    Vec3                    GetBoxSize() const;
    void                    SetBoxSize(const Vec3 &boxSize);

    Vec3                    GetBoxOffset() const;
    void                    SetBoxOffset(const Vec3 &boxOffset);

protected:
    virtual void            OnActive() override;
    virtual void            OnInactive() override;

    void                    UpdateVisuals();

    void                    TransformUpdated(const ComTransform *transform);

    ReflectionProbe::State  probeDef;
    int                     probeHandle;

    Mesh *                  sphereMesh;
    RenderObject::State     sphereDef;
    int                     sphereHandle;

    RenderWorld *           renderWorld;
};

BE_NAMESPACE_END
