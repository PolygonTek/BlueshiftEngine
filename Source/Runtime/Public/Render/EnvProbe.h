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

public:
    enum Type {
        Baked,
        Realtime
    };

    enum RefreshMode {
        OnAwake,
        EveryFrame
    };

    enum TimeSlicing {
        AllFacesAtOnce,
        IndividualFaces,
        NoTimeSlicing
    };

    enum Resolution {
        Resolution16,
        Resolution32,
        Resolution64,
        Resolution128,
        Resolution256,
        Resolution512,
        Resolution1024,
        Resolution2048
    };

    enum ClearMethod {
        ColorClear,
        SkyClear
    };

    struct State {
        Type                type = Type::Baked;
        RefreshMode         refreshMode = RefreshMode::OnAwake;
        TimeSlicing         timeSlicing = TimeSlicing::AllFacesAtOnce;

        Resolution          resolution = Resolution::Resolution128;
        bool                useHDR = true;
        ClearMethod         clearMethod = ClearMethod::SkyClear;
        Color4              clearColor = Color4::black;
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

        bool                useBoxProjection = false;

                            // Component GUID for texture hash name.
        Guid                guid;

        Texture *           bakedDiffuseProbeTexture = nullptr;
        Texture *           bakedSpecularProbeTexture = nullptr;
    };

    EnvProbe(int index);
    ~EnvProbe();

                            /// Returns type.
    Type                    GetType() const { return state.type; }

                            /// Returns AABB in world space.
    const AABB &            GetWorldAABB() const { return worldAABB; }

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

                            /// Returns prefiltered specular cube texture.
    Texture *               GetDiffuseProbeTexture() const { return diffuseProbeTexture; }

                            /// Returns irradiance diffuse cube texture.
    Texture *               GetSpecularProbeTexture() const { return specularProbeTexture; }

                            /// Returns time slicing mode. Time slicing means how the probe should distribute its updates over time. 
                            /// This is valid only in realtime type.
    TimeSlicing             GetTimeSlicing() const { return state.timeSlicing; }

                            /// Returns size.
    int                     GetSize() const { return ToActualResolution(state.resolution); }

                            /// Converts Resolution enum to actual size.
    static int              ToActualResolution(Resolution resolution);

private:
                            /// Updates this probe with the given state.
    void                    Update(const State *state);

    int                     index;              // index of probe list in RenderWorld

    State                   state;

    AABB                    worldAABB;

    bool                    needToRefresh = false;

    Texture *               diffuseProbeTexture = nullptr;
    Texture *               specularProbeTexture = nullptr;

    RenderTarget *          diffuseProbeRT = nullptr;
    RenderTarget *          specularProbeRT = nullptr;

    DbvtProxy *             proxy;
};

BE_NAMESPACE_END
