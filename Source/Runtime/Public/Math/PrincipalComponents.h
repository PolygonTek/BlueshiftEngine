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

    Extracts the principal axes, variances, and effective rank from a symmetric
    positive-semidefinite covariance matrix.

-------------------------------------------------------------------------------
*/

BE_NAMESPACE_BEGIN

template <typename VectorType, typename MatrixType>
struct BE_API PrincipalComponents {
    PrincipalComponents() { Clear(); }

                        // Resets the eigenvalues, axes, and rank to their default values.
    void                Clear();

                        // Computes the principal components from a covariance matrix.
    bool                SetFromCovariance(const MatrixType &covariance, float relativeEpsilon = 1e-5f);

    VectorType          eigenValues;
    MatrixType          axes;
    int                 rank;
};

using PrincipalComponents2 = PrincipalComponents<Vec2, Mat2>;
using PrincipalComponents3 = PrincipalComponents<Vec3, Mat3>;

BE_NAMESPACE_END
