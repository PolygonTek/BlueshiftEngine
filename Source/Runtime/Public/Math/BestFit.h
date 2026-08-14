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

/*
-------------------------------------------------------------------------------

    Best Fit

    Least-squares fitting of geometric primitives and transforms to points.

-------------------------------------------------------------------------------
*/

BE_NAMESPACE_BEGIN

class Vec3;
class Line;
class Plane;
class Mat3x4;

                    /// Fits an infinite line to the points using the dominant principal component.
                    /// The line passes through the point mean and has a normalized, deterministic direction.
                    /// On failure, line is left unchanged.
                    /// @return False if the input is invalid or the best-fit direction is not uniquely determined.
BE_API bool         BestFitLine(const Vec3 *points, int numPoints, Line &line, float relativeEpsilon = 1e-5f);

                    /// Fits a plane to the points using the least-variance principal component as its normal.
                    /// The plane passes through the point mean and has a normalized, deterministic normal.
                    /// On failure, plane is left unchanged.
                    /// @return False if the input is invalid or the best-fit normal is not uniquely determined.
BE_API bool         BestFitPlane(const Vec3 *points, int numPoints, Plane &plane, float relativeEpsilon = 1e-5f);

                    /// Finds the rigid transform that minimizes the squared distances from sourcePoints[i]
                    /// to targetPoints[i]. Point correspondences must already be known; no matching is performed.
                    /// The result contains only a proper rotation and translation, never scale or reflection.
                    /// On failure, transform is left unchanged.
                    /// @return False if the input is invalid or the rotation is not uniquely determined.
BE_API bool         BestFitRigidTransform(const Vec3 *sourcePoints, const Vec3 *targetPoints, int numPoints, Mat3x4 &transform, float relativeEpsilon = 1e-5f);

BE_NAMESPACE_END
