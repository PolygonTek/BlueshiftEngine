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

#include "Math/Matrix2.h"
#include "Math/Matrix3.h"

/*
-------------------------------------------------------------------------------

    Principal Components

    PrincipalComponents2 : Principal components of a symmetric positive-semidefinite 2x2 covariance matrix.
    PrincipalComponents3 : Principal components of a symmetric positive-semidefinite 3x3 covariance matrix.

-------------------------------------------------------------------------------
*/

BE_NAMESPACE_BEGIN

struct BE_API PrincipalComponents2 {
    PrincipalComponents2() { Clear(); }

    void                Clear();

    bool                SetFromCovariance(const Mat2 &covariance, float relativeEpsilon = 1e-5f);

    Vec2                eigenValues;
    Mat2                axes;
    int                 rank;
};

struct BE_API PrincipalComponents3 {
    PrincipalComponents3() { Clear(); }

    void                Clear();

    bool                SetFromCovariance(const Mat3 &covariance, float relativeEpsilon = 1e-5f);

    Vec3                eigenValues;
    Mat3                axes;
    int                 rank;
};

BE_NAMESPACE_END
