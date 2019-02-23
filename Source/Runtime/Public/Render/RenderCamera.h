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

    RenderCamera

-------------------------------------------------------------------------------
*/

#include "Math/Math.h"

BE_NAMESPACE_BEGIN

class RenderWorld;
class RenderLight;

class RenderCamera {
    friend class RenderWorld;

public:
    enum Flag {
        WireFrameMode       = BIT(0),
        TexturedMode        = BIT(1),
        NoShadows           = BIT(2),
        NoSubViews          = BIT(3),
        SkipPostProcess     = BIT(4),
        SkipDebugDraw       = BIT(5),
        InstantToneMapping  = BIT(6),
        ConstantToneMapping = BIT(7),
    };

    enum ClearMethod {
        NoClear,
        DepthOnlyClear,
        ColorClear,
        SkyboxClear
    };

    struct State {
        int             time = 0;               ///< Time in milliseconds for shader effects and other time dependent rendering issues
        int             flags = TexturedMode;
        int             layerMask;

        Vec3            origin;                 ///< View origin
        Mat3            axis;                   ///< View axis [FORWARD, LEFT, UP]
        Rect            renderRect;

        ClearMethod     clearMethod = NoClear;  ///< Clearing method before rendering view
        Color4          clearColor;             ///< Clearing color is needed if clearing method is set to 'ColorClear'

        float           fovX;                   ///< Perspective projection FOV in horizontal axis
        float           fovY;                   ///< Perspective projection FOV in vertical axis
        float           sizeX;                  ///< Orthogonal projection size in horizontal axis
        float           sizeY;                  ///< Orthogonal projection size in vertical axis
        float           zNear;                  ///< Near distance in z axis
        float           zFar;                   ///< Far distance in z axis
        bool            orthogonal = false;     ///< True for orthogonal projection or perspective projection
    };

    void                Update(const State *state);

                        /// Returns state.
    const State &       GetState() const { return state; }

    float               GetZNear() const { return zNear; }

    float               GetZFar() const { return zFar; }

    void                RecalcZFar(float zFar);

                        /// Transforms world coordinates to clip coordinates.
    Vec4                TransformWorldToClip(const Vec3 &worldCoords) const;

                        /// Transforms clip coordinates to pixel coordinates.
    bool                TransformClipToPixel(const Vec4 &clipCoords, Vec3 &pixelCoords) const;
    bool                TransformClipToPixel(const Vec4 &clipCoords, Point &pixelPoint) const;

                        /// Transforms world coordinates to pixel coordinates
    bool                TransformWorldToPixel(const Vec3 &worldCoords, Vec3 &pixelCoords) const;
    bool                TransformWorldToPixel(const Vec3 &worldCoords, Point &pixelPoint) const;

                        /// Calculates clipping rectangle from bounding sphere (Different camera axis with Eric Lengyel's method)
    bool                CalcClipRectFromSphere(const Sphere &sphere, Rect &clipRect) const;

                        /// Calculates clipping rectangle from axis-aligned bounding box.
    bool                CalcClipRectFromAABB(const AABB &aabb, Rect &clipRect) const;
    
                        /// Calculates clipping rectangle from oriented bounding box.
    bool                CalcClipRectFromOBB(const OBB &obb, Rect &clipRect) const;
    
                        /// Calculates clipping rectangle from bounding frustum.
    bool                CalcClipRectFromFrustum(const Frustum &frustum, Rect &clipRect) const;

    bool                CalcDepthBoundsFromPoints(int numPoints, const Vec3 *points, const Mat4 &mvp, float *depthMin, float *depthMax) const;
    bool                CalcDepthBoundsFromSphere(const Sphere &sphere, const Mat4 &mvp, float *depthMin, float *depthMax) const;
    bool                CalcDepthBoundsFromAABB(const AABB &bounds, const Mat4 &mvp, float *depthMin, float *depthMax) const;
    bool                CalcDepthBoundsFromOBB(const OBB &box, const Mat4 &mvp, float *depthMin, float *depthMax) const;
    bool                CalcDepthBoundsFromFrustum(const Frustum &frustum, const Mat4 &mvp, float *depthMin, float *depthMax) const;
    bool                CalcDepthBoundsFromLight(const RenderLight *light, const Mat4 &mvp, float *depthMin, float *depthMax) const;

    const OBB           GetBox() const { return box; }

    const Frustum &     GetFrustum() const { return frustum; }

    const Plane &       GetFrustumPlane(int index) const { return frustumPlanes[index]; }

    const Mat4 &        GetViewMatrix() const { return viewMatrix; }
    const Mat4 &        GetProjMatrix() const { return projMatrix; }
    const Mat4 &        GetViewProjMatrix() const { return viewProjMatrix; }

    static void         ComputeFov(float fromFovX, float fromAspectRatio, float toAspectRatio, float *toFovX, float *toFovY);

    static const Ray    RayFromScreenND(const RenderCamera::State &sceneView, float ndx, float ndy);

private:
    State               state;

    OBB                 box;

    Frustum             frustum;
    Plane               frustumPlanes[6];
    Vec3                frustumPoints[8];

    float               zNear;
    float               zFar;

    Mat4                viewMatrix;
    Mat4                projMatrix;
    Mat4                viewProjMatrix;

    Plane               clipPlane;
};

BE_NAMESPACE_END
