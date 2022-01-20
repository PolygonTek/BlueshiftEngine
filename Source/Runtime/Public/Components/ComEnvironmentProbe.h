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

    virtual bool            HasRenderObject(int renderObjectHandle) const override;

                            /// Called once when game started before Start()
                            /// When game already started, called immediately after spawned
    virtual void            Awake() override;

                            /// Called on game world update, variable timestep.
    virtual void            Update() override;

#if WITH_EDITOR
                            /// Visualize the component in editor
    virtual void            DrawGizmos(const RenderCamera *camera, bool selected, bool selectedByParent) override;
#endif

    virtual const AABB      GetAABB() const override;

    EnvProbe::Type::Enum    GetType() const;
    void                    SetType(EnvProbe::Type::Enum type);

    EnvProbe::RefreshMode::Enum GetRefreshMode() const;
    void                    SetRefreshMode(EnvProbe::RefreshMode::Enum refreshMode);

    EnvProbe::TimeSlicing::Enum GetTimeSlicing() const;
    void                    SetTimeSlicing(EnvProbe::TimeSlicing::Enum timeSlicing);

    int                     GetImportance() const;
    void                    SetImportance(int importance);

    EnvProbe::Resolution::Enum GetResolution() const;
    void                    SetResolution(EnvProbe::Resolution::Enum resolution);

    bool                    IsHDR() const;
    void                    SetHDR(bool useHDR);

    int                     GetLayerMask() const;
    void                    SetLayerMask(int layerMask);

    EnvProbe::ClearMethod::Enum GetClearMethod() const;
    void                    SetClearMethod(EnvProbe::ClearMethod::Enum clearMethod);

    Color3                  GetClearColor() const;
    void                    SetClearColor(const Color3 &clearColor);

    float                   GetClippingNear() const;
    void                    SetClippingNear(float clippingNear);

    float                   GetClippingFar() const;
    void                    SetClippingFar(float clippingFar);

    bool                    IsBoxProjection() const;
    void                    SetBoxProjection(bool useBoxProjection);

    Vec3                    GetBoxExtent() const;
    void                    SetBoxExtent(const Vec3 &boxExtent);

    Vec3                    GetBoxOffset() const;
    void                    SetBoxOffset(const Vec3 &boxOffset);

    float                   GetBlendDistance() const;
    void                    SetBlendDistance(float blendDistance);

    Guid                    GetBakedDiffuseProbeTextureGuid() const;
    void                    SetBakedDiffuseProbeTextureGuid(const Guid &textureGuid);

    Guid                    GetBakedSpecularProbeTextureGuid() const;
    void                    SetBakedSpecularProbeTextureGuid(const Guid &textureGuid);

    void                    ForceToRefresh();

    Texture *               GetDiffuseProbeTexture() const;
    Texture *               GetSpecularProbeTexture() const;

    Str                     WriteDiffuseProbeTexture(const Str &probesDir) const;
    Str                     WriteSpecularProbeTexture(const Str &probesDir) const;

protected:
    virtual void            OnActive() override;
    virtual void            OnInactive() override;

    void                    UpdateVisuals();

    void                    TransformUpdated(const ComTransform *transform);

    EnvProbe::State         probeDef;
    int                     probeHandle = -1;

#if WITH_EDITOR
    Mesh *                  sphereMesh = nullptr;
    RenderObject::State     sphereDef;
    int                     sphereHandle = -1;
#endif

    int                     gizmoCurrentTime = 0;
    int                     gizmoRefreshTime = 0;

    RenderWorld *           renderWorld;
};

BE_NAMESPACE_END
