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

    SceneView

-------------------------------------------------------------------------------
*/

#include "Math/Math.h"

BE_NAMESPACE_BEGIN

class SceneLight;

class SceneView {
public:
    enum Flag {
        WireFrameMode       = BIT(0),
        TexturedMode        = BIT(1),
        NoShadows           = BIT(2),
        NoSubViews          = BIT(3),
        SkipPostProcess     = BIT(4),
        SkipDebugDraw       = BIT(5)
    };

    enum ClearMethod {
        NoClear,
        DepthOnlyClear,
        ColorClear,
        SkyboxClear
    };

    struct Parms {
        int             time;               // time in milliseconds for shader effects and other time dependent rendering issues
        int             flags;
        int             layerMask;

        Rect            renderRect;
        Vec3            origin;             ///< View origin
        Mat3            axis;               ///< View axis [FORWARD, LEFT, UP]
        ClearMethod     clearMethod;        ///< Clearing method before rendering view
        Color4          clearColor;         ///< Clearing color is needed if clearing method is set to 'ColorClear'

        float           fovX;               ///< Perspective projection FOV in horizontal axis
        float           fovY;               ///< Perspective projection FOV in vertical axis
        float           sizeX;              ///< Orthogonal projection size in horizontal axis
        float           sizeY;              ///< Orthogonal projection size in vertical axis
        float           zNear;              ///< Near distance in z axis
        float           zFar;               ///< Far distance in z axis
        bool            orthogonal;         ///< True for orthogonal projection or perspective projection
    };

    void                Update(const Parms *viewParms);

    void                RecalcZFar(float zFar);

                        /// Transforms world coordinates to normalized deivice coordinates
    Vec4                WorldToNormalizedDevice(const Vec3 &worldCoords) const;
                        /// Transforms normalized device coordinates to pixel coordinates
    bool                NormalizedDeviceToPixel(const Vec4 &normalizedDeviceCoords, Vec3 &pixelCoords) const;
    bool                NormalizedDeviceToPixel(const Vec4 &normalizedDeviceCoords, Point &pixelPoint) const;
                        /// Transforms world coordinates to pixel coordinates
    bool                WorldToPixel(const Vec3 &worldCoords, Vec3 &pixelCoords) const;
    bool                WorldToPixel(const Vec3 &worldCoords, Point &pixelPoint) const;

                        /// sphere 바운딩 볼륨으로부터 clipRect 를 만든다 (Eric Lengyel's method 와 카메라 축이 다르다)
    bool                GetClipRectFromSphere(const Sphere &sphere, Rect &clipRect) const;

    bool                GetClipRectFromAABB(const AABB &aabb, Rect &clipRect) const;
    
    bool                GetClipRectFromOBB(const OBB &obb, Rect &clipRect) const;
    
    bool                GetClipRectFromFrustum(const Frustum &frustum, Rect &clipRect) const;

    bool                GetDepthBoundsFromPoints(int numPoints, const Vec3 *points, const Mat4 &mvp, float *depthMin, float *depthMax) const;
    bool                GetDepthBoundsFromSphere(const Sphere &sphere, const Mat4 &mvp, float *depthMin, float *depthMax) const;
    bool                GetDepthBoundsFromAABB(const AABB &bounds, const Mat4 &mvp, float *depthMin, float *depthMax) const;
    bool                GetDepthBoundsFromOBB(const OBB &box, const Mat4 &mvp, float *depthMin, float *depthMax) const;
    bool                GetDepthBoundsFromFrustum(const Frustum &frustum, const Mat4 &mvp, float *depthMin, float *depthMax) const;
    bool                GetDepthBoundsFromLight(const SceneLight *light, const Mat4 &mvp, float *depthMin, float *depthMax) const;

    static void         ComputeFov(float fromFovX, float fromAspectRatio, float toAspectRatio, float *toFovX, float *toFovY);

    static const Ray    RayFromScreenND(const SceneView::Parms &sceneView, float ndx, float ndy);

    Parms               parms;

    Mat4                viewMatrix;
    Mat4                projMatrix;
    Mat4                viewProjMatrix;
        
    float               zNear;
    float               zFar;

    OBB                 box;

    Frustum             frustum;
    Plane               frustumPlanes[6];
    Vec3                frustumPoints[8];

    Plane               clipPlane;
};

BE_NAMESPACE_END
