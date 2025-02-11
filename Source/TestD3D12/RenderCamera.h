// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

class RenderWorld;

class RenderCamera {
    friend class RenderWorld;

public:
    struct Decl {
        int                 time = 0;               ///< Time in milliseconds for shader effects and other time dependent rendering issues

        BE1::Rect           renderRect;
        BE1::Vec3           origin;                 ///< View origin
        BE1::Mat3           axis;                   ///< View axis [FORWARD, LEFT, UP]

        float               fovX;                   ///< Perspective projection FOV in horizontal axis
        float               fovY;                   ///< Perspective projection FOV in vertical axis
        float               sizeX;                  ///< Orthogonal projection size in horizontal axis
        float               sizeY;                  ///< Orthogonal projection size in vertical axis
        float               zNear;                  ///< Near distance in z axis
        float               zFar;                   ///< Far distance in z axis
        bool                orthogonal = false;     ///< True for orthogonal projection or false for perspective projection
    };

    void                    Update();

                            /// Returns camera input definition.
    Decl &                  GetDecl() { return decl; }
    const Decl &            GetDecl() const { return decl; }

    float                   GetZNear() const { return zNear; }
    float                   GetZFar() const { return zFar; }

    const BE1::OBB          GetBox() const { return box; }

    const BE1::Frustum &    GetFrustum() const { return frustum; }
    const BE1::Plane &      GetFrustumPlane(int index) const { return frustumPlanes[index]; }

    const BE1::Mat4 &       GetViewMatrix() const { return viewMatrix; }
    const BE1::Mat4 &       GetProjMatrix() const { return projMatrix; }
    const BE1::Mat4 &       GetViewProjMatrix() const { return viewProjMatrix; }

                            /// Transforms world coordinates to normalized device coordinates.
    bool                    TransformWorldToNDC(const BE1::Vec3 &worldCoords, BE1::Vec3 &normalizedDeviceCoords) const;

                            /// Transform normalized device coordinates to pixel coordinates.
    void                    TransformNDCToPixel(const BE1::Vec3 &normalizedDeviceCoords, BE1::Vec3 &pixelCoords) const;

                            /// Transforms world coordinates to pixel coordinates.
    bool                    TransformWorldToPixel(const BE1::Vec3 &worldCoords, BE1::Vec3 &pixelCoords) const;

                            /// Un-transform pixel coordinates to normalized device coordinates.
    void                    UntransformPixelToNDC(const BE1::Vec3 &pixelCoords, BE1::Vec3 &normalizedDeviceCoords) const;

                            /// Un-transform normalized device coordinates to world coordinates.
    bool                    UntransformNDCToWorld(const BE1::Vec3 &normalizedDeviceCoords, BE1::Vec3 &worldCoords) const;

                            /// Un-transforms pixel coordinates to world coordinates.
    bool                    UntransformPixelToWorld(const BE1::Vec3 &pixelCoords, BE1::Vec3 &worldCoords) const;

                            /// Calculates clipping rectangle from bounding sphere (Different camera axis with Eric Lengyel's method)
    bool                    CalcClipRectFromSphere(const BE1::Sphere &sphere, BE1::Rect &clipRect) const;

                            /// Calculates clipping rectangle from axis-aligned bounding box.
    bool                    CalcClipRectFromAABB(const BE1::AABB &aabb, BE1::Rect &clipRect) const;
    
                            /// Calculates clipping rectangle from oriented bounding box.
    bool                    CalcClipRectFromOBB(const BE1::OBB &obb, BE1::Rect &clipRect) const;
    
                            /// Calculates clipping rectangle from bounding frustum.
    bool                    CalcClipRectFromFrustum(const BE1::Frustum &frustum, BE1::Rect &clipRect) const;

    bool                    CalcDepthBoundsFromPoints(int numPoints, const BE1::Vec3 *points, const BE1::Mat4 &mvp, float *depthMin, float *depthMax) const;
    bool                    CalcDepthBoundsFromSphere(const BE1::Sphere &sphere, const BE1::Mat4 &mvp, float *depthMin, float *depthMax) const;
    bool                    CalcDepthBoundsFromAABB(const BE1::AABB &bounds, const BE1::Mat4 &mvp, float *depthMin, float *depthMax) const;
    bool                    CalcDepthBoundsFromOBB(const BE1::OBB &box, const BE1::Mat4 &mvp, float *depthMin, float *depthMax) const;
    bool                    CalcDepthBoundsFromFrustum(const BE1::Frustum &frustum, const BE1::Mat4 &mvp, float *depthMin, float *depthMax) const;

private:
    Decl                    decl;

    BE1::OBB                box;

    BE1::Frustum            frustum;
    BE1::Plane              frustumPlanes[6];
    BE1::Vec3               frustumPoints[8];

    float                   zNear;
    float                   zFar;

    ALIGN_AS32 BE1::Mat4    viewMatrix;
    ALIGN_AS32 BE1::Mat4    projMatrix;
    ALIGN_AS32 BE1::Mat4    viewProjMatrix;
};
