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

/*
-------------------------------------------------------------------------------

    Reflection Probe

-------------------------------------------------------------------------------
*/

BE_NAMESPACE_BEGIN

class ReflectionProbe {
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

    enum Resolution {
        Resolution16,
        Resolution32,
        Resolution64,
        Resolution128,
        Resolution256,
        Resolution1024,
        Resolution2048
    };

    enum ClearMethod {
        ColorClear,
        SkyClear
    };

    struct State {
        Type            type;
        RefreshMode     refreshMode;
        Resolution      resolution;
        bool            useHDR;
        ClearMethod     clearMethod;
        Color4          clearColor;
        float           clippingNear;
        float           clippingFar;

        int             importance;
        int             layerMask;

        Vec3            origin;

                        // Box offset from the origin.
        Vec3            boxOffset;

                        // Box extents for each axis from the origin which is translated by offset. 
                        // The origin must be included in the box range.
        Vec3            boxSize;

        bool            useBoxProjection;
    };

    ReflectionProbe(int index);
    ~ReflectionProbe();

                        /// Returns AABB in world space.
    const AABB          GetWorldAABB() const { return worldAABB; }

                        /// Returns prefiltered specular cube texture.
    Texture *           GetDiffuseSumCubeTexture() const { return diffuseSumCubeTexture; }

                        /// Returns irradiance diffuse cube texture.
    Texture *           GetSpecularSumCubeTexture() const { return specularSumCubeTexture; }

private:
                        /// Updates this probe with the given state.
    void                Update(const State *state);

    void                Invalidate();
    void                ForceToRefresh(RenderWorld *renderWorld);

    int                 index;              // index of reflection probe list in world

    State               state;

    AABB                worldAABB;

    Texture *           diffuseSumCubeTexture = nullptr;
    Texture *           specularSumCubeTexture = nullptr;

    RenderTarget *      diffuseSumCubeRT = nullptr;
    RenderTarget *      specularSumCubeRT = nullptr;

    bool                needToRefresh = false;

    DbvtProxy *         proxy;
};

BE_NAMESPACE_END
