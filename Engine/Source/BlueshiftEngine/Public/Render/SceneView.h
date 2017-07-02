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
        NoShadows           = BIT(2),
        NoSubViews          = BIT(3),
        SkipPostProcess     = BIT(4),
        SkipDebugDraw       = BIT(5)
    };

    enum ClearMethod {
        NoClear,
        DepthOnlyClear,
        ColorClear,
        SkyBoxClear
    };

    struct Parms {
        int             time;               // time in milliseconds for shader effects and other time dependent rendering issues
        int             flags;
        int             layerMask;

        Rect            renderRect;
        Vec3            origin;             // view origin
        Mat3            axis;               // view axis [FORWARD, LEFT, UP]
        ClearMethod     clearMethod;
        Color4          clearColor;

        float           fovX;
        float           fovY;
        float           sizeX;
        float           sizeY;
        float           zNear;
        float           zFar;
        bool            orthogonal;
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

    double              GetDepthFromViewZ(float viewz) const;
    bool                GetDepthBoundsFromSphere(const Sphere &sphere, double *depthMin, double *depthMax) const;
    bool                GetDepthBoundsFromAABB(const AABB &bounds, double *depthMin, double *depthMax) const;
    bool                GetDepthBoundsFromOBB(const OBB &box, double *depthMin, double *depthMax) const;
    bool                GetDepthBoundsFromFrustum(const Frustum &frustum, double *depthMin, double *depthMax) const;
    bool                GetDepthBoundsFromLight(const SceneLight *light, double *depthMin, double *depthMax) const;

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
