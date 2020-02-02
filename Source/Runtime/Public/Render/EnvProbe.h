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

#include "Core/Guid.h"

/*
-------------------------------------------------------------------------------

    Environment Probe

-------------------------------------------------------------------------------
*/

BE_NAMESPACE_BEGIN

class EnvProbeJob;

class EnvProbe {
    friend class EnvProbeJob;
    friend class RenderWorld;
    friend class RenderSystem;

public:
    struct Type {
        enum Enum {
            Baked,
            Realtime
        };
    };

    struct RefreshMode {
        enum Enum {
            OnAwake,
            EveryFrame
        };
    };

    struct TimeSlicing {
        enum Enum {
            AllFacesAtOnce,
            IndividualFaces,
            NoTimeSlicing
        };
    };

    struct Resolution {
        enum Enum {
            Resolution16,
            Resolution32,
            Resolution64,
            Resolution128,
            Resolution256,
            Resolution512,
            Resolution1024,
            Resolution2048
        };
    };

    struct ClearMethod {
        enum Enum {
            Color,
            Sky
        };
    };

    struct State {
        Type::Enum          type = Type::Baked;
        RefreshMode::Enum   refreshMode = RefreshMode::OnAwake;
        TimeSlicing::Enum   timeSlicing = TimeSlicing::AllFacesAtOnce;

        Resolution::Enum    resolution = Resolution::Resolution128;
        bool                useHDR = true;
        ClearMethod::Enum   clearMethod = ClearMethod::Sky;
        Color3              clearColor = Color3::black;
        float               clippingNear = 0.1f;
        float               clippingFar = 500.0f;

        int                 importance = 0;
        int                 layerMask = -1;

                            // Cubemap center to render.
        Vec3                origin = Vec3::origin;

                            // Box offset from the origin.
        Vec3                boxOffset = Vec3::zero;

                            // Box extents for each axis from the origin which is translated by offset.
                            // The origin must be included in the box range.
        Vec3                boxExtent = Vec3::zero;

        float               blendDistance = 1.0f;

        bool                useBoxProjection = false;

                            // Component GUID for texture hash name.
        Guid                guid;

        Texture *           bakedDiffuseProbeTexture = nullptr;
        Texture *           bakedSpecularProbeTexture = nullptr;

        int                 bounces = 0;
    };

    EnvProbe(RenderWorld *renderWorld, int index);
    ~EnvProbe();

                            /// Returns type.
    Type::Enum              GetType() const { return state.type; }

                            /// Returns proxy AABB in world space.
    const AABB &            GetProxyAABB() const { return proxyAABB; }

                            /// Returns influence AABB in world space.
    const AABB &            GetInfluenceAABB() const { return influenceAABB; }

                            /// Returns position in world space.
    const Vec3 &            GetOrigin() const { return state.origin; }

                            /// Returns box center in world space.
    const Vec3              GetBoxCenter() const { return state.origin + state.boxOffset; }

                            /// Returns box extent.
    const Vec3              GetBoxExtent() const { return state.boxExtent; }

                            /// Returns box projection.
    bool                    UseBoxProjection() const { return state.useBoxProjection; }

                            /// Returns importance for blending.
    int                     GetImportance() const { return state.importance; }

                            /// Returns diffuse probe cubemap texture.
    Texture *               GetDiffuseProbeTexture() const { return diffuseProbeTexture; }

                            /// Returns specular probe cubemap texture.
    Texture *               GetSpecularProbeTexture() const { return specularProbeTexture; }

                            /// Returns specular probe cubemap max mip level.
    int                     GetSpecularProbeTextureMaxMipLevel() const { return specularProbeTextureMaxMipLevel; }

                            /// Returns time slicing mode. Time slicing means how the probe should distribute its updates over time. 
                            /// This is valid only in realtime type.
    TimeSlicing::Enum       GetTimeSlicing() const { return state.timeSlicing; }

                            /// Returns size.
    int                     GetSize() const { return ToActualResolution(state.resolution); }

                            /// Converts Resolution enum to actual size.
    static int              ToActualResolution(Resolution::Enum resolution);

private:
                            /// Updates this probe with the given state.
    void                    Update(const State *state);

    State                   state;

    AABB                    proxyAABB;
    AABB                    influenceAABB;

    Texture *               diffuseProbeTexture = nullptr;
    RenderTarget *          diffuseProbeRT = nullptr;

    Texture *               envProbeTexture = nullptr;
    RenderTarget *          envProbeRT = nullptr;

    Texture *               specularProbeTexture = nullptr;
    int                     specularProbeTextureMaxMipLevel = 0;
    RenderTarget *          specularProbeRT = nullptr;

    int                     bounces = 0;
    bool                    needToRefresh = false;

    DbvtProxy *             proxy;
    RenderWorld *           renderWorld;
    int                     index;              // index of probe list in RenderWorld
};

BE_NAMESPACE_END
