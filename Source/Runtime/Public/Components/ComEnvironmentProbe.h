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

class ComEnvironmentProbe : public Component {
public:
    OBJECT_PROTOTYPE(ComEnvironmentProbe);

    ComEnvironmentProbe();
    virtual ~ComEnvironmentProbe();

    virtual void            Purge(bool chainPurge = true) override;

                            /// Initializes this component. Called after deserialization.
    virtual void            Init() override;

    virtual bool            HasRenderEntity(int renderEntityHandle) const override;

    virtual bool            RayIntersection(const Vec3 &start, const Vec3 &dir, bool backFaceCull, float &lastScale) const override;

                            /// Called once when game started before Start()
                            /// When game already started, called immediately after spawned
    virtual void            Awake() override;

                            /// Called on game world update, variable timestep.
    virtual void            Update() override;

                            /// Visualize the component in editor
    virtual void            DrawGizmos(const RenderCamera::State &viewState, bool selected) override;

    virtual const AABB      GetAABB() override;

    EnvProbe::Type          GetType() const;
    void                    SetType(EnvProbe::Type type);

    EnvProbe::RefreshMode   GetRefreshMode() const;
    void                    SetRefreshMode(EnvProbe::RefreshMode refreshMode);

    bool                    IsTimeSlicing() const;
    void                    SetTimeSlicing(bool timeSlicing);

    int                     GetImportance() const;
    void                    SetImportance(int importance);

    EnvProbe::Resolution    GetResolution() const;
    void                    SetResolution(EnvProbe::Resolution resolution);

    bool                    IsHDR() const;
    void                    SetHDR(bool useHDR);

    int                     GetLayerMask() const;
    void                    SetLayerMask(int layerMask);

    EnvProbe::ClearMethod   GetClearMethod() const;
    void                    SetClearMethod(EnvProbe::ClearMethod clearMethod);

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

    Guid                    GetBakedDiffuseProbeTextureGuid() const;
    void                    SetBakedDiffuseProbeTextureGuid(const Guid &textureGuid);

    Guid                    GetBakedSpecularProbeTextureGuid() const;
    void                    SetBakedSpecularProbeTextureGuid(const Guid &textureGuid);

    void                    Bake();

protected:
    Str                     WriteDiffuseProbeTexture();
    Str                     WriteSpecularProbeTexture();

    virtual void            OnActive() override;
    virtual void            OnInactive() override;

    void                    UpdateVisuals();

    void                    TransformUpdated(const ComTransform *transform);

    EnvProbe::State         probeDef;
    int                     probeHandle;

    Mesh *                  sphereMesh;
    RenderObject::State     sphereDef;
    int                     sphereHandle;

    int                     gizmoCurrentTime = 0;
    int                     gizmoRefreshTime = 0;

    RenderWorld *           renderWorld;
};

BE_NAMESPACE_END
